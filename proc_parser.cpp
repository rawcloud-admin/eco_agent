#include <sstream>
#include <string>

#include "proc_parser.h"
#include "log.h"
using namespace std;

#include "log.h"

extern int proc_number;
extern bool debug;

const char proc_dir[] = "/proc/";


int read_dir(const string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL)
    {
        stringstream ss;
        ss << "Error(" << errno << ") opening " << dir;
        log_message(ss.str());
        return -1;
    }

    while ((dirp = readdir(dp)) != NULL)
    {
        files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}


//read /proc/* dir to files
int read_proc_dir(vector<string> &files)
{
    return read_dir(string(proc_dir), files);
}

//read a task directory for a process /proc/[pid]/task/*
int read_task_dir(const string proc, vector<string> &files)
{
    return read_dir(string(proc_dir) + proc + "/task", files);
}

//read a fds directory for a process /proc/[pid]/fd/*
int read_fd_dir(const string proc, vector<string> &files)
{
    return read_dir(string(proc_dir) + proc + "/fd", files);
}

bool is_proc_dir(const string &file)
{
    if (!atol(file.c_str()))
        return false;
    return true;
}

void regulate_name(string &name)
{
    for(size_t i=0; i<name.size(); i++)
    {
        switch(name[i])
        {
            case '/':
            case ':':
            case ';':
            case '@':
                name[i] = '_';
                break;
        }
    }
}

int read_task_data(stringstream &out, const string process_dir, const string &task_dir)
{
    string fstatus = string(proc_dir) + process_dir + "/task/" + task_dir + "/status";
    ifstream pstatus;
    pstatus.open(fstatus.c_str());
    if( !pstatus.is_open() )
    {
        log_message("RTD: Unable to open status: " + fstatus);
        return -1;
    }

    THREAD th;
    string line("empty");
    int count = 4;
    while( count !=0 && line.size()>0 )
    {
        getline(pstatus, line);
        stringstream ss;
        ss.str(line);
        string name;
        string sz;
        ss >> name >> sz;
        switch(name[0])
        {
            case 'N':
                if( name == "Name:" )
                {
                    th.Name = sz;
                    regulate_name(th.Name);
                    count--;
                }
                break;
            case 'S':
                if( name == "State:" )
                {
                    th.State = sz;
                    count--;
                }
                break;
            case 'P':
                if( name == "Pid:" )
                {
                    th.Pid = sz;
                    count--;
                }
                if( name == "PPid:" )
                {
                    th.PPid = sz;
                    count--;
                }
            break;
        }
    }
    pstatus.close();


// /proc/[PID]/task/[pid]/stat
//#14 utime - CPU time spent in user code, measured in clock ticks READ
//#15 stime - CPU time spent in kernel code, measured in clock ticks READ
//#22 starttime - Time when the process started, measured in clock ticks NOREAD
    string fstat = string(proc_dir) + process_dir + "/task/" + task_dir + "/stat";
    ifstream pstat;
    pstat.open(fstat.c_str());
    if( !pstat.is_open() )
    {
        log_message("RTD: Unable to open stat: " + fstat);
        return -2;
    }

    string line2;
    getline( pstat, line2 );
    pstat.close();

    stringstream ln;
    ln.str( line2 );
    vector <string> vals;
    vals.resize( 15 );
    for( int i=0; i<15; i++ )
        ln >> vals[i];

    th.utime = vals[13];
    th.stime = vals[14];
//    th.starttime = vals[21];

    out << "@" << th.Name << "@" << th.State << "@" << th.Pid << "@" << th.PPid << "@" << th.utime << "@" << th.stime;
    return 0;
}


int read_proc_data(stringstream &out, const string &process_dir)
{
    string fstatus = string(proc_dir) + process_dir + "/status";
    ifstream pstatus;
    pstatus.open(fstatus.c_str());
    if( !pstatus.is_open() )
    {
        log_message("RPD: Unable to open status: " + fstatus);
        return -1;
    }

    PROCESS proc;
    string line("empty");
    int count = 9;
    while( count !=0 && line.size()>0 )
    {
        getline(pstatus, line);
        stringstream ss;
        ss.str(line);
        string name;
        string sz;
        ss >> name >> sz;
        switch(name[0])
        {
            case 'N':
                if( name == "Name:" )
                {
                    proc.Name = sz;
                    regulate_name(proc.Name);
                    count--;
                }
                break;
            case 'S':
                if( name == "State:" )
                {
                    proc.State = sz;
                    count--;
                }
                break;
            case 'P':
                if( name == "Pid:" )
                {
                    proc.Pid = sz;
                    count--;
                }
                break;
            case 'V':
                if( name == "VmPeak:" )
                {
                    proc.VmPeak = sz;
                    count--;
                }
                if( name == "VmSize:" )
                {
                    proc.VmSize = sz;
                    count--;
                }
                if( name == "VmRSS:" )
                {
                    proc.VmRSS = sz;
                    count--;
                }
                if( name == "VmData:" )
                {
                    proc.VmData = sz;
                    count--;
                }
                if( name == "VmStk:" )
                {
                    proc.VmStk = sz;
                    count--;
                }
                if( name == "VmExe:" )
                {
                    proc.VmExe = sz;
                    count--;
                }
                break;
            default:
                continue;
                break;
        }
    }
    pstatus.close();

// /proc/[PID]/stat
//#14 utime - CPU time spent in user code, measured in clock ticks
//#15 stime - CPU time spent in kernel code, measured in clock ticks
//#16 cutime - Waited-for children's CPU time spent in user code (in clock ticks)
//#17 cstime - Waited-for children's CPU time spent in kernel code (in clock ticks)
//#22 starttime - Time when the process started, measured in clock ticks

    string fstat = string(proc_dir) + process_dir + "/stat";
    ifstream pstat;
    pstat.open(fstat.c_str());
    if( !pstat.is_open() )
    {
        log_message("RPD: Unable to open stat: " + fstat);
        return -2;
    }

    string line2;
    getline( pstat, line2 );
    pstat.close();

    stringstream ln;
    ln.str( line2 );
    vector <string> vals;
    vals.resize( 22 );
    for( int i=0;i<22;++i )
        ln >> vals[i];

    proc.utime = vals[13];
    proc.stime = vals[14];
    proc.starttime = vals[21];

    vector<string> fdfiles;
    if( read_fd_dir(process_dir, fdfiles) )
    {
        log_message("RPD: Error reading fd directory of" + process_dir);
        return -3;
    }
    int fd_no=0;
    for( size_t i=0; i<fdfiles.size(); i++ )
    {
/*        stringstream ss;
        ss << "Process: " << process_dir << "/fd/" << fdfiles[i];
        log_message(ss.str()); */
        if( fdfiles[i] == "." || fdfiles[i] == ".." )
            continue;
        fd_no++;
    }

    out << ";" << proc.Name << ":" << proc.State << ":" << proc.Pid << ":"
        << proc.VmPeak << ":" << proc.VmSize << ":" << proc.VmRSS << ":"
        << proc.VmData << ":" << proc.VmStk << ":" << proc.VmExe << ":"
        << proc.utime << ":" << proc.stime << ":" << proc.starttime << ":" << fd_no << ":TH";

    vector<string> tfiles;
    if( read_task_dir(process_dir, tfiles) )
    {
        log_message("RPD: Error reading task directory of" + process_dir);
        return -3;
    }
    for( size_t i=0; i<tfiles.size(); i++ )
    {
        if( !is_proc_dir(tfiles[i]) )
            continue;
        read_task_data(out, process_dir, tfiles[i]);
    }

    return 0;
}
