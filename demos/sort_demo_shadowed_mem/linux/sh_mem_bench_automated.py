#!/usr/bin/python

import sys, re, string, pexpect, subprocess, getpass, time, datetime
from packagekit.enums import EXIT_SUCCESS

TIMEOUT_SEC = 300
REPEAT_COUNT = 3

EXPORT_DIR= "/exports/rootfs_mb"
RECONOS= "/home/meise/git/reconos_epics"
DOW = RECONOS+"/tools/dow"
KERNEL = "/home/meise/git/linux-2.6-xlnx/arch/microblaze/boot/simpleImage.ml605_epics"

LOG_FILE_BASELINE="baseline_run"
LOG_FILE_BASELINE_SH="baseline_sh_run"
LOG_FILE_BASELINE_SH_ON_LVL1="baseline_sh_on_lvl1_run"
LOG_FILE_BASELINE_SH_ON_LVL2="baseline_sh_on_lvl2_run"
LOG_FILE_BASELINE_SH_ON_LVL3="baseline_sh_on_lvl3_run" # makes no sense for baseline, do it for sanity checking
LOG_FILE_REL_ON_LVL1="rel_on_lvl1_run"
LOG_FILE_REL_ON_LVL2="rel_on_lvl2_run"
LOG_FILE_REL_ON_LVL3="rel_on_lvl3_run"
LOG_FILE_PERF_ON_LVL1="perf_on_lvl1_run"
LOG_FILE_PERF_ON_LVL2="perf_on_lvl2_run"
LOG_FILE_PERF_ON_LVL3="perf_on_lvl3_run"


BIT_SORT = RECONOS+"/demos/sort_demo_shadowed_mem/hw/edk/implementation/system.bit"
BIT_SORT_REL = RECONOS+"/demos/sort_demo_shadowed_mem/hw/edk_mem_rel/implementation/system.bit"
#BIT_SORT_PERF = RECONOS+"/demos/sort_demo_shadowed_mem/hw/edk_mem_perf/implementation/system.bit"
BIT_SORT_PERF = "/home/meise/Desktop/system_arb_perf_8192.bit"

SORT_DEMO_DIR="/demos/sort_demo_shadowed_mem"

SORT_PARAMS = [4,8,16,32,64,128,256,512]

sort_commands_baseline = ["./sort_demo -h 1 -s 0 -b "+str(x)+" >> {}/sort_"+LOG_FILE_BASELINE+" 2>&1" for x in SORT_PARAMS]
sort_commands_baseline += ["./sort_demo_shadowed -h 1 -s 0 -b "+str(x)+" >> {}/sort_"+LOG_FILE_BASELINE_SH+" 2>&1" for x in SORT_PARAMS]
sort_commands_baseline += ["./sort_demo_shadowed -h 1 -s 0 -b "+str(x)+" -a --level 1 >> {}/sort_"+LOG_FILE_BASELINE_SH_ON_LVL1+" 2>&1" for x in SORT_PARAMS]
sort_commands_baseline += ["./sort_demo_shadowed -h 1 -s 0 -b "+str(x)+" -a --level 2 >> {}/sort_"+LOG_FILE_BASELINE_SH_ON_LVL2+" 2>&1" for x in SORT_PARAMS]
sort_commands_baseline += ["./sort_demo_shadowed -h 1 -s 0 -b "+str(x)+" -a --level 3 >> {}/sort_"+LOG_FILE_BASELINE_SH_ON_LVL3+" 2>&1" for x in SORT_PARAMS]

#LOG_FILE_REL_OFF="rel_off_run"

sort_commands_rel = ["./sort_demo_shadowed -h 1 -s 0 -b "+str(x)+" -a  --level 1 >> {}/sort_"+LOG_FILE_REL_ON_LVL1 +" 2>&1" for x in SORT_PARAMS] 
sort_commands_rel +=["./sort_demo_shadowed -h 1 -s 0 -b "+str(y)+" -a  --level 2 >> {}/sort_"+LOG_FILE_REL_ON_LVL2 +" 2>&1" for y in SORT_PARAMS]
sort_commands_rel +=["./sort_demo_shadowed -h 1 -s 0 -b "+str(y)+" -a  --level 3 >> {}/sort_"+LOG_FILE_REL_ON_LVL3 +" 2>&1" for y in SORT_PARAMS]

#LOG_FILE_PERF_OFF="perf_off_run"

sort_commands_perf = ["./sort_demo_shadowed -h 1 -s 0 -b "+str(x)+" -a  --level 1 >> {}/sort_"+LOG_FILE_PERF_ON_LVL1 +" 2>&1" for x in SORT_PARAMS] 
sort_commands_perf +=["./sort_demo_shadowed -h 1 -s 0 -b "+str(y)+" -a  --level 2 >> {}/sort_"+LOG_FILE_PERF_ON_LVL2 +" 2>&1" for y in SORT_PARAMS]
sort_commands_perf +=["./sort_demo_shadowed -h 1 -s 0 -b "+str(y)+" -a  --level 3 >> {}/sort_"+LOG_FILE_PERF_ON_LVL3 +" 2>&1" for y in SORT_PARAMS]


BIT_MATRIX = RECONOS+"/demos/matrixmul_shadowed_mem/hw/edk_linux/implementation/system.bit"
BIT_MATRIX_REL = RECONOS+"/demos/matrixmul_shadowed_mem/hw/edk_linux_mem_rel/implementation/system.bit"
BIT_MATRIX_PERF = RECONOS+"/demos/matrixmul_shadowed_mem/hw/edk_linux_mem_perf/implementation/system.bit"
MATRIX_DEMO_DIR="/demos/matrixmul_shadowed_mem"

MATRIX_PARAMS = [256,512] #,1024]

matrix_commands_baseline = ["./matrixmul -f -h 1 -s 0 -b "+str(x)+" >> {}/matrix_"+LOG_FILE_BASELINE+" 2>&1" for x in MATRIX_PARAMS]
matrix_commands_baseline += ["./matrixmul_shadowed -f -h 1 -s 0 -b "+str(x)+" >> {}/matrix_"+LOG_FILE_BASELINE_SH+" 2>&1" for x in MATRIX_PARAMS]
matrix_commands_baseline += ["./matrixmul_shadowed -f -h 1 -s 0 -b "+str(x)+" -a --level 1 >> {}/matrix_"+LOG_FILE_BASELINE_SH_ON_LVL1+" 2>&1" for x in MATRIX_PARAMS]
matrix_commands_baseline += ["./matrixmul_shadowed -f -h 1 -s 0 -b "+str(x)+" -a --level 2 >> {}/matrix_"+LOG_FILE_BASELINE_SH_ON_LVL2+" 2>&1" for x in MATRIX_PARAMS]
matrix_commands_baseline += ["./matrixmul_shadowed -f -h 1 -s 0 -b "+str(x)+" -a --level 3 >> {}/matrix_"+LOG_FILE_BASELINE_SH_ON_LVL3+" 2>&1" for x in MATRIX_PARAMS]

#LOG_FILE_REL_OFF="rel_off_run"

matrix_commands_rel = ["./matrixmul_shadowed -f -h 1 -s 0 -b "+str(x)+" -a  --level 1 >> {}/matrix_"+LOG_FILE_REL_ON_LVL1 +" 2>&1" for x in MATRIX_PARAMS] 
matrix_commands_rel +=["./matrixmul_shadowed -f -h 1 -s 0 -b "+str(y)+" -a  --level 2 >> {}/matrix_"+LOG_FILE_REL_ON_LVL2 +" 2>&1" for y in MATRIX_PARAMS]
matrix_commands_rel +=["./matrixmul_shadowed -f -h 1 -s 0 -b "+str(y)+" -a  --level 3 >> {}/matrix_"+LOG_FILE_REL_ON_LVL3 +" 2>&1" for y in MATRIX_PARAMS]

#LOG_FILE_PERF_OFF="perf_off_run"

matrix_commands_perf = ["./matrixmul_shadowed -f -h 1 -s 0 -b "+str(x)+" -a  --level 1 >> {}/matrix_"+LOG_FILE_PERF_ON_LVL1 +" 2>&1" for x in MATRIX_PARAMS] 
matrix_commands_perf +=["./matrixmul_shadowed -f -h 1 -s 0 -b "+str(y)+" -a  --level 2 >> {}/matrix_"+LOG_FILE_PERF_ON_LVL2 +" 2>&1" for y in MATRIX_PARAMS]
matrix_commands_perf +=["./matrixmul_shadowed -f -h 1 -s 0 -b "+str(y)+" -a  --level 3 >> {}/matrix_"+LOG_FILE_PERF_ON_LVL3 +" 2>&1" for y in MATRIX_PARAMS]



def downloadStuff(_bitstreamOrKernel):
    return subprocess.call([DOW, _bitstreamOrKernel])
    
def startBenchmark(_benchmarkTag, _telnetPasswd):
    child = pexpect.spawn ('telnet 192.168.35.2')
    child.expect ('reconos login:.*')
    child.sendline ('root')
    child.expect ('Password:.*')
    child.sendline (_telnetPasswd)
    child.expect('# ')
    child.sendline('cd /demos/sort_demo_shadowed_mem')
    child.expect('# ')
    child.sendline('./sh_mem_benchmark.sh ' + _benchmarkTag)
    child.expect('# ', timeout=None) # benchmark can take a long time, so we deactivate the timeout
    child.sendline('exit')

class ReturnValueError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

def runBenchmark(_benchmarkTag, _telnetPasswd, _commands, _work_dir):

    # LOGIN
    child = pexpect.spawn ('telnet 192.168.35.2')
    child.expect ('reconos login:.*')
    child.sendline ('root')
    child.expect ('Password:.*')
    child.sendline (_telnetPasswd)
    
    # Preparations
    child.expect('# ')
    child.sendline('cd '+ _work_dir)
    
    child.expect('# ')
    child.sendline('mkdir -p '+ _benchmarkTag)
    child.expect('# ')
    child.sendline('chmod o+rw '+ _benchmarkTag)
    child.expect('# ')
    
    # Benchmarks
    for i in xrange(REPEAT_COUNT):
        for cmd in _commands:   
            tryAgain = True;
            while tryAgain:
                print('I' + str(i) + ' ' + time.ctime()+ ' ' +cmd.format(_benchmarkTag))
                child.sendline(cmd.format(_benchmarkTag))
                  
                response = child.expect(['# ', pexpect.TIMEOUT],timeout=TIMEOUT_SEC)
                if response == 1: # on timeout
                    # abort programm via CTRL-C
                    child.sendcontrol('c') # send 3 times just to be sure
                    child.sendcontrol('c')
                    child.sendcontrol('c')
                    child.expect('# ',timeout=TIMEOUT_SEC)
                    # insert abortion message into logfile
                    log_file = cmd.format(_benchmarkTag).split('>>',1)[1]
                    print ("Program aborted due to timeout. Logging to {}".format(log_file))
                    
                    child.sendline('echo "PROGRAM ABORTED!  TIMEOUT EXCEEDED!" >> ' + log_file)
                    child.expect('# ',timeout=TIMEOUT_SEC)
                    #child.sendline('echo -e "\tSort data    : -1 ms" >> '+ log_file)
                    continue
                    
                # test error code 
                child.sendline('echo $?') 
                #time.sleep(5)
                response= child.expect(['echo \$\?\r\n0\r\n# ','echo \$\?\r\n[1-9][0-9]*\r\n# ', pexpect.TIMEOUT],timeout=TIMEOUT_SEC)
                if response ==1: # On error return code
                    print(child.before)
                    print(child.after)
                    #log_file = cmd.format(_benchmarkTag).split(' ')[10]
                    log_file = cmd.format(_benchmarkTag).split('>>',1)[1]
                    print ("Return Code {0} indicated error. Logging to {1}".format(child.after.split('\r\n')[1], log_file))
                    child.sendline('echo "PROGRAM ABORTED!  RETURNCODE INDICATED ERROR: {0}" >> '.format(child.after.split('\r\n')[1]) + log_file)
                    child.expect('# ',timeout=TIMEOUT_SEC)
                    continue
                elif response == 2: # on timeout
                    print(child.before)
                    print(child.after)
                    # abort programm via CTRL-C
                    child.sendcontrol('c') # send 3 times just to be sure
                    child.sendcontrol('c')
                    child.sendcontrol('c')
                    child.expect('# ',timeout=TIMEOUT_SEC)
                    # insert abortion message into logfile
                    log_file = cmd.format(_benchmarkTag).split('>>',1)[1]
                    print ("Status query aborted due to timeout. Logging to {}".format(log_file))
                    
                    child.sendline('echo "STATUS QUERY FAILED!  TIMEOUT EXCEEDED!" >> ' + log_file)
                    child.expect('# ',timeout=TIMEOUT_SEC)
                    #child.sendline('echo -e "\tSort data    : -1 ms" >> '+ log_file)
                    continue        
                
                tryAgain = False                        
    # Exit
    child.sendline('exit')
    child.expect('Connection closed by foreign host.')

def benchmark(_tagBase, _telnetPasswd, _bitstream, _commands, _work_dir):
    print("Downloading Bitstream...")
    downloadStuff(_bitstream)
    print("Downloading Kernel...")
    downloadStuff(KERNEL)
    print("Waiting for system boot...")
    time.sleep(60)
    print("Executing Benchmark...")
    runBenchmark(_tagBase, _telnetPasswd, _commands, _work_dir)

if __name__ == "__main__":
    telnetPasswd = getpass.getpass('telnet password: ')
    
    benchmarkTagBase = str(datetime.date(1,1,1).today())
    
    #
    # Benchmarks
    #
    
    #benchmark(benchmarkTagBase, telnetPasswd, BIT_SORT,      sort_commands_baseline,   SORT_DEMO_DIR)
    #benchmark(benchmarkTagBase, telnetPasswd, BIT_SORT_REL,  sort_commands_rel,        SORT_DEMO_DIR)
    #benchmark(benchmarkTagBase, telnetPasswd, BIT_SORT_PERF, sort_commands_perf,        SORT_DEMO_DIR)
    
    benchmark(benchmarkTagBase, telnetPasswd, BIT_MATRIX,      matrix_commands_baseline,    MATRIX_DEMO_DIR)
    benchmark(benchmarkTagBase, telnetPasswd, BIT_MATRIX_REL,  matrix_commands_rel,         MATRIX_DEMO_DIR)
    benchmark(benchmarkTagBase, telnetPasswd, BIT_MATRIX_PERF, matrix_commands_perf,        MATRIX_DEMO_DIR)
    
    print("Done!")
    sys.exit(EXIT_SUCCESS)
    