/*
 * reconos.c
 *
 *  Created on: Apr 22, 2013
 *      Author: meise
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <mb_interface.h>
#include "reconos.h"


void mbox_put(struct mbox *mb, uint32_t msg){
	uint32_t retval;
	putfsl(OSIF_CMD_MBOX_PUT, OSIF_FSL);
	putfsl(mb->handle, OSIF_FSL);
	putfsl(msg, OSIF_FSL);
	getfsl(retval, OSIF_FSL);
}

uint32_t mbox_get(struct mbox *mb){
	uint32_t retval;
	putfsl(OSIF_CMD_MBOX_GET, OSIF_FSL);
	putfsl(mb->handle, OSIF_FSL);
	getfsl(retval, OSIF_FSL);
	return retval;
}

/**
 * @param size gives maximum size of message we can accept.
 * @return gives how many bytes we really did receive
 */
ssize_t rq_receive(rqueue *rq, uint32_t *msg, size_t size){
	int i;
	uint32_t recv_bytes;

	putfsl(OSIF_CMD_RQ_RECEIVE, OSIF_FSL);
	putfsl(*rq, OSIF_FSL);
	putfsl(size, OSIF_FSL);

	getfsl(recv_bytes, OSIF_FSL);
	for (i = 0; i < recv_bytes / sizeof(uint32_t); ++i)
			getfsl(msg[i], OSIF_FSL);

	return recv_bytes;
}

void rq_send(rqueue *rq, uint32_t *msg, size_t size){
	int i;
	uint32_t retval;

	putfsl(OSIF_CMD_RQ_SEND, OSIF_FSL);
	putfsl(*rq, OSIF_FSL);
	putfsl(size, OSIF_FSL);

	for (i = 0; i < size / sizeof(uint32_t); ++i)
			putfsl(msg[i], OSIF_FSL);

	getfsl(retval, OSIF_FSL);
}

int sem_wait (sem_t *__sem){
	uint32_t retval;
	putfsl(OSIF_CMD_SEM_WAIT, OSIF_FSL);
	putfsl(*__sem, OSIF_FSL);
	getfsl(retval, OSIF_FSL);
	return retval;
}

int sem_post (sem_t *__sem){
	uint32_t retval;
	putfsl(OSIF_CMD_SEM_POST, OSIF_FSL);
	putfsl(*__sem, OSIF_FSL);
	getfsl(retval, OSIF_FSL);
	return retval;
}


int pthread_mutex_trylock (pthread_mutex_t *__mutex){
	uint32_t retval;
	putfsl(OSIF_CMD_MUTEX_TRYLOCK, OSIF_FSL);
	putfsl(*__mutex, OSIF_FSL);
	getfsl(retval, OSIF_FSL);
	return retval;
}

int pthread_mutex_lock (pthread_mutex_t *__mutex){
	uint32_t retval;
	putfsl(OSIF_CMD_MUTEX_LOCK, OSIF_FSL);
	putfsl(*__mutex, OSIF_FSL);
	getfsl(retval, OSIF_FSL);
	return retval;
}

int pthread_mutex_unlock (pthread_mutex_t *__mutex){
	uint32_t retval;
	putfsl(OSIF_CMD_MUTEX_UNLOCK, OSIF_FSL);
	putfsl(*__mutex, OSIF_FSL);
	getfsl(retval, OSIF_FSL);
	return retval;
}


int pthread_cond_wait (pthread_cond_t *__restrict __cond,
			      pthread_mutex_t *__restrict __mutex){
	uint32_t retval;
	putfsl(OSIF_CMD_COND_WAIT, OSIF_FSL);
	putfsl(*__cond, OSIF_FSL);
	putfsl(*__mutex, OSIF_FSL);
	getfsl(retval, OSIF_FSL);
	return retval;
}

int pthread_cond_signal (pthread_cond_t *__cond){
	uint32_t retval;
	putfsl(OSIF_CMD_COND_SIGNAL, OSIF_FSL);
	putfsl(*__cond, OSIF_FSL);
	getfsl(retval, OSIF_FSL);
	return retval;
}

int pthread_cond_broadcast (pthread_cond_t *__cond){
	uint32_t retval;
	putfsl(OSIF_CMD_COND_BROADCAST, OSIF_FSL);
	putfsl(*__cond, OSIF_FSL);
	getfsl(retval, OSIF_FSL);
	return retval;
}

/**
 * Exits thread.
 * @note: Return value is ignored; Delegate thread does always return NULL.
 */
void pthread_exit(void *retval){
	putfsl(OSIF_CMD_THREAD_EXIT, OSIF_FSL);
	exit(EXIT_SUCCESS);
}

void pthread_yield(){
	uint32_t retval;
	putfsl(OSIF_CMD_THREAD_YIELD, OSIF_FSL);
	getfsl(retval, OSIF_FSL);
}

/**
 * Return pointer into main memory, where init data is stored.
 */
void * reconos_get_init_data(){
	void* retval;
	putfsl(OSIF_CMD_THREAD_GET_INIT_DATA, OSIF_FSL);
	getfsl(retval, OSIF_FSL);
	return retval;
}


/**
 * Copies data from local ram into system's main memory.
 * @note: Only multiples of 4 for len parameter allowed, because FSL copies always 32 bits.
 * @implementation At time of implementation, memory subsystem accepts non aligned addresses, but only chunks up to 256 bytes of length.
 */
#define MAX_REQUEST_LEN_BYTES 256
void memif_write(const void* src_addr, void* dst_addr, uint32_t len){
	uint32_t request_len;
	uint32_t temp;
	uint32_t i;

	while (len > 0){
		request_len = len <= MAX_REQUEST_LEN_BYTES ? len : MAX_REQUEST_LEN_BYTES;
		len -= request_len;

		temp = (MEMIF_CMD_WRITE << 24) | // Top 8 bit are command
			   (request_len & 0x00FFFFFc);	  // lower 24 bit are length in bytes. Since only multiples of 4 bytes are allowed, lower 2 bits get zeroed.
		putfsl(temp,MEMIF_FSL);

		temp = (uint32_t) dst_addr & 0xFFFFFFFcl;  // Address of data in main memory. Lower two bits zeroed to align to word size.
		putfsl(temp,MEMIF_FSL);
		dst_addr += request_len;

		for( i=0; i< request_len; i+=4){
			temp = *((uint32_t*)src_addr);
			putfsl( temp,MEMIF_FSL);
			src_addr +=4;
		}
	}
}

/**
 * Helper functione: reverses bits in a doubleword: uint32_t
 */
uint32_t reverse_uint32_t(uint32_t b) {
	b = (b & 0xFF00FF00) >> 8 | (b & 0x00FF00FF) << 8;
	b = (b & 0xF0F0F0F0) >> 4 | (b & 0x0F0F0F0F) << 4;
	b = (b & 0xCCCCCCCC) >> 2 | (b & 0x33333333) << 2;
	b = (b & 0xAAAAAAAA) >> 1 | (b & 0x55555555) << 1;
	return b;
}

/**
 * Copies data from system's main memory into local ram.
 * @note: Only multiples of 4 for len parameter allowed, because FSL copies always 32 bits.
 * @implementation At time of implementation, memory subsystem accepts non aligned addresses, but only chunks up to 256 bytes of length.
 */
void memif_read(const void* src_addr, void* dst_addr, uint32_t len){
	uint32_t request_len;
	uint32_t temp;
	uint32_t i;

	while (len > 0){
		request_len = len <= MAX_REQUEST_LEN_BYTES ? len : MAX_REQUEST_LEN_BYTES;
		len -= request_len;

		temp = (MEMIF_CMD_READ << 24) |  // Top 8 bit are command
			   (request_len & 0x00FFFFFC);	  // lower 24 bit are length in bytes. Since only multiples of 4 bytes are allowed, lower 2 bits get zeroed.
		putfsl(temp,MEMIF_FSL);

		temp = (uint32_t) src_addr & 0xFFFFFFFcl;  // Address of data in main memory. Lower two bits zeroed to align to word size.
		putfsl(temp,MEMIF_FSL);
		src_addr += request_len;

		for( i=0; i< request_len; i+=4){
			getfsl(temp, MEMIF_FSL);
			*((uint32_t*)dst_addr) = temp;
			dst_addr +=4;
		}
	}
}