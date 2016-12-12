#include "thread.h"

#ifndef _AIPU_SENDFLV_
#define _AIPU_SENDFLV_

#include "Rtmp.h"

#define VDECODER_BUFFER_SIZE 2*1024*1024
#define ADECODER_BUFFER_SIZE 2048

//	typedef enum {
//	    AIPU_KEY = 1,
//		AIPU_P = 2,
//		AIPU_B = 3,
//	    AIPU_UNKNOWN_FRAME = -1
//	}AIPU_FRAME_TYPE;


typedef enum {
    AIPU_SPS = 7,
	AIPU_PPS = 8,
	AIPU_SEI = 6,
	AIPU_IDR = 5,
	AIPU_NON_IDR = 1,
    AIPU_UNKNOWN_NAL = -1
}AIPU_AVC_NAL;

typedef enum {
    AIPU_AAC = 0,
    AIPU_MP3 = 1,
	AIPU_G711A = 2,
	AIPU_G711U = 3,
	AIPU_H264 = 4,
    AIPU_UNKNOWN_CODEC
}AIPU_AVCODEC_TYPE;

typedef enum {
    AIPU_SUCCESS = 0,
	AIPU_FAILURE = -1,
    AIPU_OP_ORDER_ERR = -2,    
	AIPU_IN_PARA_ERR = -3,
	AIPU_NOT_SUPPORT = -4,
	AIPU_NO_RES_ERR = -5,  //no resource is available, ex. encoder channel 
	AIPU_INNER_ERR = -6
}AIPU_RET;

typedef enum {
    AIPU_KEY_FRAME = 0,
	AIPU_P_FRAME = 1,
	AIPU_B_FRAME = 2,
	AIPU_AUDIO_FRAME = 3,
	AIPU_FIND_KEY_FRAME = 4,
    AIPU_UNKNOWN_FRAME
}AIPU_FRAME_TYPE;

typedef struct
{    
	int haveVideo;
	int width;
	int height;
	int frameRate;
    AIPU_AVCODEC_TYPE videoType;
	//unsigned char profileId;
	//unsigned char levelId;
	//unsigned char profileCompat;

	int haveAudio;
	int nSamplesPerSecond;
	int nBitsPerSample;
	int nSamplePerFrm;
	int nChannels;
	AIPU_AVCODEC_TYPE audioType;
	
}flvContainer;

typedef struct
{
	unsigned char *startPtr;
	int frameLen;
}aFrameNode;

typedef struct
{
	unsigned char *startPtr;
	int frameType;
	int frameLen;
}vFrameNode;


//TFTYPE playBackStreamSending(void *arg);
int play_AddFrameData(void* handle, const int channel, const int stream, const unsigned char* data, const int len, int bKeyFrame, int rate);
int sendMetaData( RTMP *rtmp, RTMPPacket *avPacket,  int duation_sec);

//TFTYPE liveStreamSending(void *arg);

#endif











































