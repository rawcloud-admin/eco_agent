#include "proc_parser.h"

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <cerrno>

#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
using namespace std;

#include "log.h"
#include "system.h"

extern unsigned long long int clk_tick;
extern int proc_number;

#define BUFFER_SIZE 256

#define KEY_FILENAME "../public.pem"

const char pid_file[]="pid.file";
bool debug = 1;

int padding = RSA_PKCS1_PADDING;
 

RSA * createRSAWithFilename(const char *filename, int public_key)
{
    FILE * fp = fopen(filename, "r");
 
    if(fp == NULL)
    {
        log_message("Unable to open file " + string(filename));
        log_message( string(strerror(errno)) );
        return NULL;    
    }
    RSA *rsa= RSA_new() ;
 
    if(public_key)
    {
        rsa = PEM_read_RSA_PUBKEY(fp, &rsa,NULL, NULL);
    }
    else
    {
        rsa = PEM_read_RSAPrivateKey(fp, &rsa,NULL, NULL);
    }
    fclose(fp); 
    return rsa;
}

int public_encrypt(unsigned char * data, int data_len, const char *key_file, unsigned char *encrypted)
{
    RSA * rsa = createRSAWithFilename(key_file, 1);
    int result = RSA_public_encrypt(data_len,data,encrypted,rsa,padding);
    RSA_free(rsa);
    return result;
}


void printLastError(char *msg)
{
    char * err = (char*)malloc(130);;
    ERR_load_crypto_strings();
    ERR_error_string(ERR_get_error(), err);
    log_message("ERROR: " + string(msg) + string(err));
    free(err);
}

void *dostuff (void *sockp)
{
    int sock = *((int*)sockp);

    char buffer[BUFFER_SIZE+1];
    int buffer_size = 0;

    bzero(buffer,BUFFER_SIZE+1);
    while(1)
    {
        //incercam sa citim ceva din socket
        char newchar = 0x00;
        int n = read(sock,&newchar,1);
        if( n < 0 )
        {
            log_message("ERROR reading from socket. Abort!");
            break;
        }
        if( n == 0 )
        {
            //the socket is closed. just abort
            break;
        }
        if( n != 1 )
        {
            //get more than 1 char. Strange and abort
            log_message("ERROR Get more than 1 char from socket. Abort!");
            break;
        }
        //am primit 1 char to use
        if( buffer_size < BUFFER_SIZE )
        {
            //il adaug in buffer daca e diferit de 0x0D
            if( newchar != 0x0D )
            {
                buffer[buffer_size] = newchar;
                buffer_size++;
            }
        }
        if( newchar != '\n' )
            continue;

        if( strcmp(buffer, "GETONE\n") )
        {
            log_message("ERROR Unsupported command received");
            bzero(buffer,BUFFER_SIZE+1);
            buffer_size = 0;
            continue;
        }
        //we have GETONE command received
        bzero(buffer,BUFFER_SIZE+1);
        buffer_size = 0;

        SYS_STAT sys_stat;
        if( read_cpu_stat(sys_stat) )
        {
            log_message("THREAD: Error reading CPU stat");
            break;
        }
        if( read_sys_mem(sys_stat) )
        {
            log_message("THREAD: Error reading MEM stat");
            break;
        }
        if( read_loadavg(sys_stat) )
        {
            log_message("THREAD: Error reading LOADAVG stat");
            break;
        }
        if( read_uptime(sys_stat) )
        {
            log_message("THREAD: Error reading UPTIME stat");
            break;
        }

        time_t tm = time(NULL);
        stringstream ss;
        ss << "SYS:" << sys_stat.user << ":" << sys_stat.nice << ":" << sys_stat.system
           << ":" << sys_stat.idle << ":" << sys_stat.iowait << ":" << sys_stat.irq
           << ":" << sys_stat.softirq << ":" << sys_stat.MemTotal
           << ":" << sys_stat.MemFree << ":" << clk_tick << ":" << sys_stat.MemBuffers
           << ":" << sys_stat.MemCached << ":" << sys_stat.SwapTotal << ":" << sys_stat.SwapFree
           << ":" << sys_stat.SwapCached << ":" << tm << ":" << proc_number << ":" << sys_stat.loadavg
           << ":" << sys_stat.uptime
           << ";PROCS";

        vector<string> files;
        if( read_proc_dir(files) )
        {
            log_message("THREAD: Error reading proc directory. Abort!");
            break;
        }
        size_t skipped=0l, nr_procs=0l;
        for( size_t i=0; i<files.size(); i++ )
        {
            if( !is_proc_dir(files[i]) )
                continue;
            nr_procs++;
            if( read_proc_data(ss, files[i]) )
            skipped++;
        }

        ss << endl;
        
        stringstream ss_size;
        ss_size << ss.str().size() << endl;

        n = send(sock, ss_size.str().c_str(), ss_size.str().size(), MSG_EOR|MSG_NOSIGNAL);
        if (n < 0)
        {
            log_message( "ERROR writing size of plain text to socket" );
            break;
        }

        //encryption
        unsigned char encrypted[50000] = {};
        bzero((char*)encrypted, 50000);
        unsigned char  encrypted_slice[4098]={};

        int encrypted_slice_length = 0;

        //slice
        int k, j = 0;
        int chunk_num = 0;
        int rest_num = 0;
        string ss_slice;
        int ss_slice_len = 0;

        chunk_num = ss.str().size() / 240;
        rest_num  = ss.str().size() % 240;

        //start slice
        for(k=0; k<chunk_num + 1; k++)
        {
            bzero((char*)encrypted_slice, 4098);
            ss_slice_len = 240;
            if(k == chunk_num)
            {
                ss_slice_len = rest_num;
            }
            ss_slice = ss.str().substr(k*240, ss_slice_len);
            
            //encrypt slice
            encrypted_slice_length= public_encrypt((unsigned char*)ss_slice.c_str(), ss_slice_len, KEY_FILENAME, encrypted_slice);
            if(encrypted_slice_length == -1)
            {
                printLastError("Public Encrypt failed \n");
                break;
            }

            //send slice size
            stringstream slice_size;
            slice_size << encrypted_slice_length << endl;
            n = send(sock, slice_size.str().c_str(), slice_size.str().size(), MSG_EOR|MSG_NOSIGNAL);
            if (n < 0)
            {
                log_message( "ERROR sending slice size of encrypted text to socket" );
                break;
            }

            //send slice encrypted text
            n = send(sock, encrypted_slice, encrypted_slice_length, MSG_EOR|MSG_NOSIGNAL);
            if (n < 0)
            {
                log_message("ERROR sending encrypted slice text to socket");
                break;
            }
        }
    }

    close(sock);
    return NULL;
}


void signal_handler(int sig)
{
    switch(sig)
    {
        case SIGHUP:
            log_message("Signal: Hangup signal catched");
            break;
        case SIGQUIT:
        case SIGINT:
        case SIGTERM:
            log_message("Signal: We'll terminate the program now...");
            unlink(pid_file);
            log_message("Signal: Now exit");
            exit(0);
            break;
        default:
            log_message("Signal: Unknown signal catched");
            break;
    }
}


int check_ip_reach_us(const char* input_perf_server, const char *domain_name) {

    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET_ADDRSTRLEN];

    // Set up hints
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4 only
    hints.ai_socktype = SOCK_STREAM;

    // Resolve the hostname
    if ((status = getaddrinfo(domain_name, NULL, &hints, &res)) != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(status));
        return 0;
    }

    printf("IP addresses for %s:\n\n", domain_name);

    // Iterate through the list and print the IP addresses
    for(p = res; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // Get the pointer to the address itself
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        addr = &(ipv4->sin_addr);
        ipver = "IPv4";

        // Convert the IP to a string and print it
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("%s: %s\n", ipver, ipstr);

        
        if (!strcmp(input_perf_server, ipstr)) {
            return 1;
        }
    }

    freeaddrinfo(res); // Free the linked list

    return 0;
}

// -wwork_dir, -pPORT, -D =non demonise
int main(int argc, char *argv[])
{
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    char perf_multi[20];

    string work_dir = "/chroot/systat";
    int portno = 0;
    bool daemonise = true;
    bool has_option_work_dir=false;
    bool has_option_port = false;
    bool has_option_ip = false;
    
    for(int i=1; i<argc; i++)
    {
        if(argv[i][0] != '-')
        {
            cerr << "Options: Illegal command line parameter " << argv[i] << endl;
            exit(-1);
        }
        switch(argv[i][1])
        {
            case 'w':
                has_option_work_dir = true;
                work_dir = string(&argv[i][2]);
                cout << "Work dir: " << work_dir << endl;
                break;
            case 'p':
                has_option_port = true;
                portno = atoi(&argv[i][2]);
                cout << "I will use port " << portno << " to listen for commands" << endl;
                break;
            case 'i':
                has_option_ip = true;
                strcpy(perf_multi, &argv[i][2]);
                if (perf_multi == NULL) {
                    cout << "Options: ERROR, no such host: " << &argv[i][2] << endl;
                    exit(-1);
                }
                cout << "Using " << perf_multi << " as target" << endl;
                break;
            case 'D':
                daemonise = false;
                cout << " I will not daemonise" << endl;
                break;
            default:
                cerr << "Options: Illegal command line option -" << argv[i][1] << endl;
                exit(-1);
                break;
        }
    }
    if( !has_option_work_dir || !has_option_port || !has_option_ip )
    {
        cerr << "You must provide at least work dir file (-w), port (-p) and ip (-i) parameters in command line" << endl;
        exit(-1);
    }

    if( chdir(work_dir.c_str()) )
    {
        cerr << "Main: ERROR, cannot change dir to " << work_dir << endl;
        exit(-2);
    }

    if( log_init() )
    {
        cerr << "ERROR, log file initialization" << endl;
        exit(-3);
    }

    if (daemonise) {
         log_message("Before daemon");
         int rc = daemon(1,0);
         if( rc )
         {
            log_message("ERROR: daemonise function returned errors");
            return 2;
         }
         log_message("I'm daemon now");
    }

    if( read_proc_number() )
        return 3;

    signal(SIGTSTP, SIG_IGN); /* ignore tty signals */
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGHUP, signal_handler);  /* catch hangup signal */
    signal(SIGTERM, signal_handler); /* catch kill signal */
    signal(SIGQUIT, signal_handler); /* catch quit signal */
    signal(SIGINT, signal_handler);  /* catch interrupt signal */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        log_message("ERROR opening socket");
    log_message("I opened the socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    ofstream of;
    of.open(pid_file, ios::trunc);
    if (of.is_open())
    {
        of << getpid() << endl;
        of.flush();
        of.close();
    }
    else
    {
        log_message("ERROR: Unable to create the pid file");
        return 3;
    }

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
              log_message("ERROR on binding");
              log_message( string(strerror(errno)));
    }

    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
        {
            log_message("ERROR on accept");
            continue;
        }
        struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&cli_addr;
        struct in_addr ipAddr = pV4Addr->sin_addr;
        char rawMonitor_ip[INET_ADDRSTRLEN];
        inet_ntop( AF_INET, &ipAddr, rawMonitor_ip, INET_ADDRSTRLEN );

        log_message("rawMonitor ip: " + string(rawMonitor_ip ));
        //if (strcmp(rawMonitor_ip, perf_multi))
        if (!check_ip_reach_us(rawMonitor_ip, perf_multi))
        {
            log_message( "Another rawMonitor server wants to get data from you: ");
            continue;
        }

        pthread_t thr;
        pthread_create(&thr, 0, &dostuff, &newsockfd);
        pthread_detach(thr);
    }
    close(sockfd);
    return 0;
}
