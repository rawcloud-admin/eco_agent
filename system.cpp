#include <sstream>
#include <string>
#include <unistd.h>

#include "log.h"
#include "system.h"

using namespace std;

int proc_number = 0;
unsigned long long int clk_tick = 0ll;

int read_proc_number(void)
{
    clk_tick = sysconf(_SC_CLK_TCK);

    ifstream cpuinfo("/proc/cpuinfo", ifstream::in);
    if( cpuinfo.is_open() )
    {
        proc_number = 0;
        string line;
        while( getline( cpuinfo, line ) )
        {
            if( line.find("processor") != string::npos )
                proc_number++;
        }
        if( proc_number == 0 )
            proc_number = 1;

        stringstream ss;
        ss << "INFO Procesor number is " << proc_number;
        log_message( ss.str() );
        return 0;
    }

    return -1;
}

int read_cpu_stat(SYS_STAT &stat)
{
    string cpuline;
    ifstream pstat ("/proc/stat", std::ifstream::in);
    if( !pstat.is_open() )
    {
        log_message("RCS: ifstream is not opened");
        return -1;
    }

    getline (pstat, cpuline);
    pstat.close();

    stringstream sline;
    sline.str(cpuline);

    string cpu;
    sline >> cpu;
    sline >> stat.user;
    sline >> stat.nice;
    sline >> stat.system;
    sline >> stat.idle;
    sline >> stat.iowait;
    sline >> stat.irq;
    sline >> stat.softirq;

    return 0;
}

int read_sys_mem(SYS_STAT &stat)
{
    ifstream meminfo("/proc/meminfo", std::ifstream::in);
    if( !meminfo.is_open() )
    {
        log_message("RSS: ifstream is not opened");
        return -1;
    }

    string line("empty");
    int count = 7;
    while( count !=0 && line.size()>0 )
    {
        getline(meminfo, line);
        stringstream ss;
        ss.str(line);
        string name;
        string sz;
        ss >> name >> sz;
        if( name == "MemTotal:" )
        {
            stat.MemTotal = sz;
            count--;
            continue;
        }
        else if( name == "MemAvailable:" )
        {
            stat.MemFree = sz;
            count--;
            continue;
        }
        else if( name == "Buffers:" )
        {
            stat.MemBuffers = sz;
            count--;
            continue;
        }
        else if( name == "Cached:" )
        {
            stat.MemCached = sz;
            count--;
            continue;
        }
        else if( name == "SwapTotal:" )
        {
            stat.SwapTotal = sz;
            count--;
            continue;
        }
        else if( name == "SwapFree:" )
        {
            stat.SwapFree = sz;
            count--;
            continue;
        }
        else if( name == "SwapCached:" )
        {
            stat.SwapCached = sz;
            count--;
            continue;
        }
    }

    meminfo.close();
    return 0;
}

int read_sys_time(time_t &tm)
{
    tm = time(NULL);
    return 0;
}

int read_loadavg(SYS_STAT &stat)
{
    string loadline;
    ifstream file ("/proc/loadavg", std::ifstream::in);
    if( !file.is_open() )
    {
        log_message("RLA: ifstream is not opened");
        return -1;
    }

    getline (file, stat.loadavg);
    file.close();

    return 0;
}

int read_uptime(SYS_STAT &stat)
{
    string uline;
    ifstream file ("/proc/uptime", std::ifstream::in);
    if( !file.is_open() )
    {
        log_message("RUT: ifstream is not opened");
        return -1;
    }

    getline (file, stat.uptime);
    file.close();

    return 0;
}
