#include "ivtRPC.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "ivtCb.h"
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

int publishRun;
extern int timeOutS;
extern int firmwareSec;
extern int alarmPort;
extern int snap;

void sigIntHandler(int sig)
{
    IVT_DEBUG("******sddddddddddddddd****int***********\n");
	publishRun = 0;
}

static void daemonize(void)
{
    fprintf(stderr, "IVT start to daemonize\n");
    pid_t child_pid = 0;
    
    child_pid = fork();
    if(child_pid < 0){
        perror("IVT fork failed");
        exit(-1);        
    }else if(child_pid == 0){
        //in the child process
        return;
    }else{
        //in the parent process
        
        //wait for the child process
        while(1){
            pid_t ret;
            ret = waitpid(child_pid, NULL, 0);
            if(ret < 0){
                perror("waitpid error, reboot system");
                break; //on error, reboot
            }else if(ret == child_pid){
                IVT_ERR("IVT terminated, reboot system\n");
                break; //child terminated abnormal
            }
        }

        //send reboot signal to sofia
        while(1){
            reboot();
            sleep(600); //wait for reboot
        }

    }//child_pid = fork();

    //never get here
}

static inline void random_sleep(int min_ms, int max_ms)
{
    long long ms_to_sleep = (rand() % (max_ms - min_ms)) + min_ms;    
    usleep(ms_to_sleep * 1000);   
}

int main(int argc, char **argv)
{
    int i;
	char ivtMode[FM_STRING_LEN];

#if(DEBUG_IVT)
	char wsUrl[513]= "ws://116.62.180.77:25000/ivc?login_code=debugger&login_passwd=debugger"\
		"&project=demo&hardware_model=IPC-2829B&firmware_model=2.13.11.2.R5302.";
    //char wsUrl[513]= "ws://116.62.180.77:9999/ivc?login_code=debugger&login_passwd=debugger"\
		"&project=demo&hardware_model=IPC-2829B&firmware_model=2.13.11.2.R5302.";
#else
    char wsUrl[513];
    char code[64];
	char pw[64];
	char ip[64];
	int port;
	int enable;
#endif
	snap = 1;
    memset(http_host, 0, HTTP_HOST_LEN);
    http_port = 0;
	alarmPort = 9000;
	ivtRPCWebSocket *rpc = new  ivtRPCWebSocket;
    int auto_reboot = 0;
    struct timeval tv;
    long long sleep_max_ms = 0;
    
	timeOutS = 15;
    gettimeofday(&tv, NULL);
	srand((unsigned int)((tv.tv_sec << 20) | (tv.tv_usec & 0xfffff)));
    firmwareSec = rand()%(10*60);
    while(argc > 1)
	{
        if (strcmp("-httpHost", argv[1]) == 0 && 2 < argc)
        {
            strncpy(http_host, argv[2], HTTP_HOST_LEN);
            http_host[HTTP_HOST_LEN-1] = 0;
            ++argv; --argc;
        }
		else if (strcmp("-httpPort", argv[1]) == 0 && 2 < argc)
		{
			http_port = atoi(argv[2]);
            ++argv; --argc; 
		}
		else if(strcmp("-t", argv[1]) == 0 && 2 < argc)
		{
			timeOutS = atoi(argv[2]);
            ++argv; --argc;
		}
		else if(strcmp("-s", argv[1]) == 0 && 2 < argc)
		{
			sleep_max_ms = atoi(argv[2]);
            ++argv; --argc;
		}        
        else if(strcmp("-auto-reboot", argv[1]) == 0)
        {
            auto_reboot = 1;
        }
		else if(strcmp("-h", argv[1]) == 0)
        {
            IVT_ERR("ivt [-httpHost (sofia_http_host)] [-httpPort (sofia_http_port)] [-t (timeOut Second)] [-auto-reboot] [-s max_start_sleep_time(ms)]\n");
            exit(-1);
        }
        else
        {
            IVT_ERR("Unknown arg %s\n", argv[1]);
            IVT_ERR("ivt [-httpHost (sofia_http_host)] [-httpPort (sofia_http_port)] [-t (timeOut Second)] [-auto-reboot] [-s max_start_sleep_time(ms)]\n");
            exit(-1);            
        }
            
        ++argv; --argc;
	}

#ifdef UCLINUX
	if(http_port <=0)
	{
		//IVT_ERR("http_port:%d\n", http_port);
		//goto ERR_EXIT;
        char data[16];
        memset(data,0,16);
        FILE *f = NULL;
        f = fopen("/mnt/mtd/Config/httpPort", "rb");

        if (f)
        {
			int ilen = fread(data, sizeof(char), 16, f);
            if(ilen > 0)
            {
                http_port = atoi(data);
            }

            fclose(f);
        }
	}
#endif

    if (http_host[0] == 0)
    {
        IVT_ERR("http_host err: %s set default\n", "127.0.0.1");
        strcpy(http_host, "127.0.0.1");
    }

    if (http_port <=0 || http_port >= 65536)
    {
		IVT_ERR("http_port err: %d set default\n", 80);
        http_port = 80;
    }
    
#if(!DEBUG_IVT)
    while(1)
    {
        sleep(5);
		
        if(getUrl(code, pw, ip, &port, &enable, &snap, 64)<0)
        {
            IVT_ERR("getUrl ERR\n");
            continue;
        }

        if(!enable)
        {
            IVT_ERR("enable 0\n");
            return -1;
        }
        
        if(getFirmAndMode(ivtFirmware, ivtMode, FM_STRING_LEN)<0)
        {
            IVT_ERR("getFirmAndMode err\n");
            continue;
        }
		
		alarmPort = getAlarmPort();
		if(alarmPort < 0)
		{
		    IVT_ERR("getAlarmPort err\n");
            continue;
		}

        break;
    }
    if(auto_reboot){
        daemonize();  //parent process never return
    }
    
    if(sleep_max_ms){
        random_sleep(0, sleep_max_ms);
    }
    
    sprintf(wsUrl,"ws://%s:%d/ivc?login_code=%s&login_passwd=%s"\
		"&hardware_model=%s&firmware_model=%s", ip, port, code, pw, ivtMode, ivtFirmware);
    

#endif

	for(i=0;i<IVT_ELSE_EVENT;i++)
	{
		rpc->setIvtEventCallback(ivtEventCb[i], i);
	}

	for(i=0;i<IVT_ELSE_METHOD;i++)
	{
		rpc->setIvtReqCallback(ivtResCb[i], i);
	}

	for(i=0;i<IVC_ELSE_EVENT;i++)
	{
		rpc->setIvcEventCallback(ivcEventCb[i], i);
	}

	for(i=0;i<IVC_ELSE_METHOD;i++)
	{
		rpc->setIvcReqCallback(ivcReqCb[i], i);
	}
    
    ivc_rtmpStopPublishAll();
	ivc_stopCRCbAll();
	IVT_DEBUG("ws url: %s\n",wsUrl);
	while(1)
	{
		if(IVT_SUCCESS==rpc->startRPC(wsUrl))
			break;
        random_sleep(5000, 10000);
	}

    publishRun = 1;
    signal(SIGINT, sigIntHandler); //ctrl c

#ifndef WIN32
	signal(SIGPIPE, SIG_IGN);//ignore SIGPIPE
#endif

	while(publishRun)
	{
	    int reStart=0;
		sleep(5);
		mutex_lock(rpc->m_ctx->gMutex);
		reStart = rpc->m_ctx->socketErr;
		mutex_unlock(rpc->m_ctx->gMutex);

		if(reStart)
		{   
            int i = 0;
			rpc->stopRPC();	
			IVT_DEBUG("reStart\n");
			ivc_rtmpStopPublishAll();			
            IVT_DEBUG("reStart ws url: %s\n",wsUrl);
 #define QUICK_RESTSART_TIME 2           
            for(i=0;i<QUICK_RESTSART_TIME;i++){
                if(rpc->startRPC(wsUrl) == IVT_SUCCESS){
                    break;
                }
                random_sleep(1000, 3000);  
            }
            if(i == QUICK_RESTSART_TIME){ // quick restart failed   
                ivc_stopCRCbAll();
                while(rpc->startRPC(wsUrl)<0)
                    random_sleep(5000, 10000);                    
            }
		}
	}

    rpc->stopRPC();
	ivc_rtmpStopPublishAll();
	ivc_stopCRCbAll();
//ERR_EXIT:
	delete rpc;
	return 0;
}





















































