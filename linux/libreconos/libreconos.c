
#include "reconos.h"
#include "fsl.h"
#include "mbox.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#if 1
#define RECONOS_DEBUG(...) fprintf(stderr,__VA_ARGS__);
#else
#define RECONOS_DEBUG(...)
#endif

#define RECONOS_ERROR(...) fprintf(stderr,"ERROR:" __VA_ARGS__);

#define FSL_PATH_LEN 256

struct reconos_process reconos_proc;


static void slot_reset(int num, int reset){
	uint32 cmd;
	uint32 mask = 0;
	int i;
	
	if (reset) {
		reconos_proc.slot_flags[num] |= SLOT_FLAG_RESET;
	} else {
		reconos_proc.slot_flags[num] &= ~SLOT_FLAG_RESET;
	}
	
	for(i = MAX_SLOTS - 1; i >= 0; i--){
		mask = mask << 1;
		if((reconos_proc.slot_flags[i] & SLOT_FLAG_RESET)){
			mask = mask | 1;
		}
	}
	
	cmd = mask | 0x01000000;
	
	fsl_write(reconos_proc.proc_control_fsl,cmd);
}

uint32 getpgd()
{
	int res,fd;
	uint32 pgd;

	fd = open("/dev/getpgd",O_RDONLY);
	if(fd == -1){
		perror("open /dev/getpgd");
		exit(1);
	}

	res = read(fd,&pgd,4);
	if(res != 4){
		perror("read from /dev/getpgd");
		exit(1);
	}
	
	RECONOS_DEBUG("PGD = 0x%08X\n",pgd);

	close(fd);
	
	return pgd;
}

void * control_thread_entry(void * arg)
{
	RECONOS_DEBUG("control thread listening on fsl %d\n",reconos_proc.proc_control_fsl);
	while(1){
		uint32 cmd;
		uint32 ret;
		uint32 *addr;

		cmd = fsl_read(reconos_proc.proc_control_fsl);
		RECONOS_DEBUG("control thread received 0x%08X\n", cmd);
		
		/* for now, all we receive here is page fault addresses */
		addr = (uint32*)cmd;
		
		ret = *addr; /* access memory */
		
		ret = ret & 0x00FFFFFF; /* clear upper 8 bits */
		ret = ret | 0x03000000; /* set page ready command */

		fsl_write(reconos_proc.proc_control_fsl,ret); /* Note: the lower 24 bits of ret are ignored by the HW. */
	}
}

int reconos_init(int proc_control_fsl)
{
	int i;
	uint32 pgd;
	pthread_attr_t attr;

	reconos_proc.proc_control_fsl = proc_control_fsl;
	for(i = 0; i < MAX_SLOTS; i++){
		reconos_proc.slot_flags[i] |= SLOT_FLAG_RESET;
	}
	
	fsl_write(proc_control_fsl,0x04000000);

	pgd = getpgd();

	fsl_write(proc_control_fsl,0x02000000);
	fsl_write(proc_control_fsl,pgd);

	pthread_attr_init(&attr);	
	pthread_create(&reconos_proc.proc_control_thread, NULL, control_thread_entry,NULL);
	
	return 0;
}


void * delegate_thread_entry(void * arg);

int reconos_hwt_create(
		struct reconos_hwt * hwt,
		int slot,
		void * arg)
{
	hwt->slot = slot;
	return pthread_create(&hwt->delegate,NULL,delegate_thread_entry,hwt);
}

void reconos_hwt_setresources(struct reconos_hwt * hwt, struct reconos_resource * res, int num_resources)
{
	hwt->resources = res;
	hwt->num_resources = num_resources;
}



void * delegate_thread_entry(void * arg)
{
	struct reconos_hwt * hwt;
	
	hwt = (struct reconos_hwt *)arg;

	RECONOS_DEBUG("slot %d: starting delegate thread\n", hwt->slot);
	
	slot_reset(hwt->slot,1);
	slot_reset(hwt->slot,0);
	
	while(1){
		uint32 cmd;
		uint32 handle;
		uint32 arg0;
		uint32 result;
		
		cmd = fsl_read(hwt->slot);
		
		RECONOS_DEBUG("slot %d: received command 0x%08X\n", hwt->slot, cmd);
		
		switch(cmd){
			case RECONOS_CMD_MBOX_GET:
				RECONOS_DEBUG("slot %d: command is MBOX_GET\n", hwt->slot);
				handle = fsl_read(hwt->slot);
				RECONOS_DEBUG("slot %d: resource id is 0x%08X\n", hwt->slot, handle);
			
				if(handle >= hwt->num_resources){
					RECONOS_ERROR("slot %d: resource id %d out of range, must be lesser than %d\n",
						hwt->slot, handle, hwt->num_resources);
					exit(1);
				}
				
				if(hwt->resources[handle].type != RECONOS_TYPE_MBOX){
					RECONOS_ERROR("slot %d: resource type 0x%08X expected, found 0x%08X\n",
						hwt->slot, RECONOS_TYPE_MBOX, hwt->resources[handle].type);
					exit(1);
				}
				
			
				result = mbox_get(hwt->resources[handle].ptr);
				RECONOS_DEBUG("slot %d: mbox_get returns 0x%08X\n", hwt->slot, result);
				
				fsl_write(hwt->slot, result);
				break;	
			
			case RECONOS_CMD_MBOX_PUT:
				RECONOS_DEBUG("slot %d: command is MBOX_PUT\n", hwt->slot);
				handle = fsl_read(hwt->slot);
				RECONOS_DEBUG("slot %d: resource id is 0x%08X\n", hwt->slot, handle);
				arg0 = fsl_read(hwt->slot);
				RECONOS_DEBUG("slot %d: data is 0x%08X\n", hwt->slot, arg0);
				
				if(handle >= hwt->num_resources){
					RECONOS_ERROR("slot %d: resource id %d out of range, must be lesser than %d\n",
						hwt->slot, handle, hwt->num_resources);
					exit(1);
				}
				
				if(hwt->resources[handle].type != RECONOS_TYPE_MBOX){
					RECONOS_ERROR("slot %d: resource type 0x%08X expected, found 0x%08X\n",
						hwt->slot, RECONOS_TYPE_MBOX, hwt->resources[handle].type);
					exit(1);
				}
				
				mbox_put(hwt->resources[handle].ptr, arg0);
				RECONOS_DEBUG("slot %d: mbox_put returns void\n", hwt->slot);
				
				fsl_write(hwt->slot, 0);
				break;
			
			default:
				RECONOS_ERROR("slot %d: unknown command 0x%08X\n", hwt->slot, cmd);
				exit(1);
		}
	}
		
	return NULL;
}



