#ifndef PROC_PARSER_H
#define PROC_PARSER_H
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

using namespace std;

int read_proc_dir (vector<string> &files);
bool is_proc_dir(const string &file);
int read_proc_data(stringstream &out, const string &process_dir);


struct sthread {
    sthread():Name("no_name"), State("0"), Pid("-1"), PPid("-1"), utime("0"), stime("0")
    {}

    string Name;
    string State;
    string Pid;
    string PPid;
    string utime;
    string stime;
};
typedef struct sthread THREAD;


struct proc {
    proc():Name("no_name"), State("0"), Pid("-1"), VmPeak("0"), VmSize("0"),
    VmRSS("0"), VmData("0"), VmStk("0"), VmExe("0"), utime("0"), stime("0"),
    starttime("0")
    {
        vthreads.clear();
    }

    string Name;
    string State;
    string Pid;
    string VmPeak;
    string VmSize;
    string VmRSS;
    string VmData;
    string VmStk;
    string VmExe;
    string utime;
    string stime;
    string starttime;

    vector <THREAD> vthreads;
};
typedef struct proc PROCESS;



#endif // PROC_PARSER_H
