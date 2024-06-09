#include <fstream>
#include <time.h>
#include <string>

using namespace std;

char log_file[]="log.file";

int log_message(string msg)
{
    ofstream of;
    of.open(log_file, ios::out|ios::app);
    if (of.is_open())
    {
        time_t rawtime = time(0);
        string date_now = string(ctime(&rawtime));
        date_now[date_now.size()-1] = ' ';
        of << date_now << msg << endl;
        of.close();
        return 0;
    }
    return -1;
}


int log_init()
{
    return log_message("Init log file");
}


