#ifndef SYSTAT_SYSTEM_H
#define SYSTAT_SYSTEM_H

#include <sys/types.h>
#include <cstdlib>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>


struct sys_stat_struct {
    sys_stat_struct():user(0l), nice(0l), system(0l),
                      idle(0l), iowait(0l), irq(0l), softirq(0l),
                      loadavg("0"), uptime("0")
    {}

    unsigned long long int user;   //time spent in user mode
    unsigned long long int nice;   //time spent in user mode with lowe priority
    unsigned long long int system; //time spent in system mode
    unsigned long long int idle;   //time spent in idle task
    unsigned long long int iowait; //Time waiting for I/O to complete
    unsigned long long int irq;    //Time servicing interrupts
    unsigned long long int softirq;//Time servicing softirq

    string MemTotal; //physical total memory in the system
    string MemFree;  //physical free memory in the system
    string MemBuffers; //physical memory used for buffers
    string MemCached; //hpy. mem used
    string SwapTotal; //swap total memory
    string SwapFree; //swap free memory
    string SwapCached; //Memory that once was swapped out, is swapped back in
                       //but still also is in the swap file.
    string loadavg; //first line from /proc/loadavg without \n
    string uptime; //uptime line from /proc/uptime
};
typedef struct sys_stat_struct SYS_STAT;

int read_proc_number();
int read_cpu_stat(SYS_STAT &stat);
int read_sys_mem(SYS_STAT &stat);
int read_loadavg(SYS_STAT &stat);
int read_uptime(SYS_STAT &stat);

#endif // SYSTAT_SYSTEM_H
