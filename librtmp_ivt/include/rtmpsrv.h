#include "thread.h"
#include "stdio.h"
#include "list.h"

#ifndef _AIPU_RTMPSRV_
#define _AIPU_RTMPSRV_

#define AV_CHANNEL 0x14
#define BUF_DELAY_TUNE_MS (10*1000)
#define WINDOW_SIZE (1024*1024)
#define HALF_WINDOW_SIZE (WINDOW_SIZE>>1)
#define CHUNK_SIZE 1460

#define AV_PACKET_SIZE (1024*(1024+512))
#define RTMPDUMP_VERSION "2.4"
extern volatile int sever_Run;
TFTYPE doServe(void *arg);

int startStreaming(const char *address, int port);
int SendUnpauseResult(RTMP *r);
int SendPauseResult(RTMP *r);
//global variable for maintain server 

typedef struct
{  
	int playType; // //1 live 2 playBack 0 error
	int startPlay;
	//int channel; //!通道从0开始
	//int stream;  //!从0开始

	int resetFlg;
	int bufferInMs;
	int startSessoin;
	int pause;
    //int winAck;
	//int readSize;
	RTMP *rtmpSession;
	RTMPPacket avPacket;
	FILE *fp;
	int isEof;
	//THANDLE  sendThread;
	MUTEX mutexSend;
	char fileName[256];
	char *connect;

	//!视频live处理句柄
	void* live_source_handle;
	struct 	list_head 	server_list;

	RTMP_PLAY_INFO play_info;

	int64_t startMs;
	unsigned int vTimeS;
	unsigned int aTimeS;
	unsigned int sampleRate;
	int sendSps;

	int bSendAudioCfg; //!1--已经发送音频配置
} STREAMING_SERVER;

#endif

struct 	list_head* GetServerList();
MUTEX* GetServerListMutex();

typedef long long int64;

extern int64 Milliseconds();





