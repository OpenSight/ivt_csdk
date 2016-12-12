#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <assert.h>
#include "rtmp_sys.h"
#include "log.h"
#include "thread.h"
#include "rtmpsrv.h"

#include "Rtmp_vf.h"
#include "publishLive.h"

#ifdef linux
#include <linux/netfilter_ipv4.h>
#endif

#ifndef WIN32
#include <sys/types.h>
#include <sys/wait.h>
#else
#pragma comment(lib,"ws2_32.lib")

#endif


#ifdef WIN32
#define InitSockets()	{\
	WORD version;			\
	WSADATA wsaData;		\
	\
	version = MAKEWORD(1,1);	\
	WSAStartup(version, &wsaData);	}

#define	CleanupSockets()	WSACleanup()
#else
#define InitSockets()
#define	CleanupSockets()
#endif

#define RD_SUCCESS		0
#define RD_FAILED		1
#define RD_INCOMPLETE		2


static RTMP_LIVE_SOURCE s_live_stream_source;
int RtmpSetLiveSource(RTMP_LIVE_SOURCE* live_source)
{
	memcpy(&s_live_stream_source, live_source, sizeof(RTMP_LIVE_SOURCE));
	return 0;
}


RTMP_LIVE_SOURCE* RtmpGetLiveSource()
{
	return &s_live_stream_source;
}


void sigIntHandler(int sig)
{
	RTMP_ctrlC = TRUE;
	sever_Run = 0;

	while(!sever_Run)
		msleep(25);

	RTMP_LogPrintf("Caught signal: %d, cleaning up, just a second...\n", sig);
	exit(-1);
	//signal(SIGINT, SIG_DFL);
}

int rtmp_Start(int port)
{
	//int i;

	// http streaming server
	char DEFAULT_HTTP_STREAMING_DEVICE[] = "172.16.35.38";	// 0.0.0.0 is any device

	char *rtmpStreamingDevice = DEFAULT_HTTP_STREAMING_DEVICE;	// streaming device, default 0.0.0.0
	int nRtmpStreamingPort = port;//1935;	// port


	RTMP_debuglevel = RTMP_LOGINFO;

#if 0
	signal(SIGINT, sigIntHandler); //ctrl c
#ifndef WIN32
	signal(SIGPIPE, SIG_IGN);//PIPE OR SOCKET OPERATE ERROR
#endif
#endif


	InitSockets();

	//start http streaming
	printf("Streaming on rtmp://%s:%d\n", rtmpStreamingDevice,
		nRtmpStreamingPort);

	if(startStreaming(/*rtmpStreamingDevice*/NULL, nRtmpStreamingPort) == 0)
	{
		RTMP_Log(RTMP_LOGERROR, "Failed to start RTMP server, exiting!");
		return RD_FAILED;
	}



	//printf("Done, exiting...");


	//CleanupSockets();


	sever_Run = 1;
	//while(1)
		//msleep(50);
	return 0;
}

//PUBLISH 相关接口
void *RtmpPublishLiveOpen(char *url)
{
    if (url == NULL)
    {
        return NULL;
    }
    
    rtmpPublisher *pPublisher = malloc(sizeof(rtmpPublisher));
    memset(pPublisher,0,sizeof(rtmpPublisher));

	InitSockets();
	if(publishLive_open(url, pPublisher)<0)
	{
    	free(pPublisher);
		CleanupSockets();
		return NULL;
	}
    
    return pPublisher;
}

int RtmpPublishLiveClose(void *phandle)
{
    if (phandle == NULL)
    {
        return -1;
    }

    publishLive_close(phandle);
    return 0;    
}

int RtmpPublishLiveSendAV(void *phandle, const unsigned char* videoData, const int frameLen, int frmType, int frmRate)
{
    if (phandle == NULL)
    {
        return -1;
    }

    return publishLive_sendAV(phandle, videoData, frameLen, frmType, frmRate);
}

