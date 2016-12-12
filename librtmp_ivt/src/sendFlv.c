#include "sendFlv.h"
#include "Rtmp.h"
#include "Rtmpsrv.h"
#include "Log.h"
#include <string.h>
#include <list.h>
#include <assert.h>

#include "Rtmp_sys.h"
#include <sys/time.h>

#ifndef WIN32
#include <stdlib.h>

#endif

//#include "rtmp_sys.h"
extern int SendPlayStop(RTMP *r);

unsigned int adts_sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};

int64 Milliseconds()
{
    int64 curTime;
    struct timespec tsNow;
    clock_gettime(CLOCK_MONOTONIC, &tsNow);

    curTime = tsNow.tv_sec;
    curTime *= 1000;                // sec -> msec
    curTime += tsNow.tv_nsec/1000000;    // usec -> msec
    return curTime;
}

/*
int waitRelative(int64_t reltime, pthread_cond_t *timerCond, pthread_mutex_t *timerLock)
{

	struct timespec ts;
	struct timeval t;
	
	gettimeofday(&t, NULL);
	ts.tv_sec = t.tv_sec;
	ts.tv_nsec= t.tv_usec*1000;

	ts.tv_sec += reltime/1000000000;
	ts.tv_nsec+= reltime%1000000000;
	if (ts.tv_nsec >= 1000000000) {
	ts.tv_nsec -= 1000000000;
	ts.tv_sec  += 1;
	}
	
	return -pthread_cond_timedwait(timerCond, timerLock, &ts);
}*/


int readOneNALUFromEs(unsigned char *pStream, int *pFrameLen, FILE *streamFp, int *nalu_type, unsigned char **pStreamStart)
{
    int readSize, i, retVal=-1;
    unsigned char *inBuf = pStream;
    unsigned int code = 0xFFFFFFFF;
    int nalStart=-1, nalEnd=-1;
    int tmp, j=0;
	
    readSize = fread(pStream, 1, VDECODER_BUFFER_SIZE, streamFp);
    *pFrameLen = 0;
    *nalu_type = -1;
    *pStreamStart = NULL;

    for(i=0;i<readSize;i++)
    {
        code = (code<<8) + inBuf[i];
		tmp = code & 0xffffff1f;
		j++;
        if (tmp == 0x107 || tmp == 0x101
			|| tmp == 0x105 || tmp ==0x108 || tmp == 0x106)
        {   //sps 0x00 00 01 0x7  non idr  0x00 00 01 0x1 idr 0x 00 00 01 05 pps 0x00 00 01 0x8
            //sei 0x00 00 01 0x6
            *nalu_type = inBuf[i]&0x1f;
            *pStreamStart = inBuf+i;
            nalStart = i;//i-4
            break;
        }
		
    }

    if(-1!=nalStart)
    {
        code = 0xFFFFFFFF;
        for(i=nalStart+1; i<readSize; i++)
        {
            code = (code<<8) + inBuf[i];			
			tmp = code & 0xffffff1f;
			
            if (tmp == 0x107 || tmp == 0x101 
				|| tmp == 0x105 || tmp ==0x108 || tmp == 0x106 )
            {	//sps 0x00 00 01 0x7  non idr  0x00 00 01 0x1 idr 0x 00 00 01 05 pps 0x00 00 01 0x8    
            //sei 0x00 00 01 0x6
                nalEnd = i-3;//i-4
				j=j-3;
                *pFrameLen =nalEnd - nalStart;
                break;
            }
			j++;
        }
        if(readSize==i)
            *pFrameLen =readSize - nalStart;
        retVal = 0;
    }
    
    if(readSize>0)
        fseek(streamFp, j-readSize, SEEK_CUR);

    return retVal;
}

int readNALUFromIDR(unsigned char *pStream, int readSize, int naluType, int *pFrameLen, unsigned char **pStreamStart)
{
    int i, retVal=-1;
    unsigned char *inBuf = pStream;
    unsigned int code = 0xFFFFFFFF;
    int nalStart=-1, nalEnd=-1;
    int tmp;
	
    *pFrameLen = 0;
	naluType |= 0x100;
    //*nalu_type = -1;
    *pStreamStart = NULL;

    for(i=0;i<readSize;i++)
    {
        code = (code<<8) + inBuf[i];
		tmp = code & 0xffffff1f;
        if (tmp == naluType)
        {   //sps 0x00 00 01 0x7   idr 0x 00 00 01 05 pps 0x00 00 01 0x8
            //sei 0x00 00 01 0x6
            *pStreamStart = inBuf+i;
            nalStart = i;//i-4
            break;
        }		
    }

    if(-1!=nalStart)
    {
        code = 0xFFFFFFFF;
        for(i=nalStart+1; i<readSize; i++)
        {
            code = (code<<8) + inBuf[i];			
			tmp = code & 0xffffff1f;
			
            if (tmp == 0x107 || tmp == 0x105 || tmp ==0x108 || tmp == 0x106 )
            {	//sps 0x00 00 01 0x7  idr 0x 00 00 01 05 pps 0x00 00 01 0x8    
            //sei 0x00 00 01 0x6
                nalEnd = i-3;//i-4
                *pFrameLen =nalEnd - nalStart;
                break;
            }
        }
        if(readSize==i)
            *pFrameLen =readSize - nalStart;
        retVal = 0;
    }
    
    return retVal;
}


TFTYPE playLiveStreamSending2(STREAMING_SERVER *server, const unsigned char* video_data, const int video_len, int bKeyFrame, int rate);

int play_AddFrameData(void* handle, const int channel, const int stream, const unsigned char* data, const int len, int bKeyFrame, int rate)
{
	//!遍历发送所有客户端
	struct list_head *p;
	STREAMING_SERVER *d;
	
	mutex_lock(*GetServerListMutex());
	
	__list_for_each(p, GetServerList()) 
	{
	  	d = list_entry(p, STREAMING_SERVER, server_list);
		if(d->live_source_handle == handle)
		{
			playLiveStreamSending2(d, data, len, bKeyFrame, rate);
			break;
		}
	}
		
	mutex_unlock(*GetServerListMutex());

	return 0;
}

int sendPacket(RTMPPacket *avPacket, RTMP *rtmp, int frmType, 
	                    int vTimeS, unsigned char *pVStream, int frameLen)
{
    int i;
	unsigned char *payLoadPtr =  (unsigned char *)avPacket->m_body;
	i = 0;
	payLoadPtr[i++] = frmType;// 2:Pframe	7:AVC	
	payLoadPtr[i++] = 0x01;// AVC NALU	 
	payLoadPtr[i++] = 0x00;  
	payLoadPtr[i++] = 0x00;  
	payLoadPtr[i++] = 0x00;  

	// NALU size   
	payLoadPtr[i++] = frameLen>>24 &0xff;  
	payLoadPtr[i++] = frameLen>>16 &0xff;  
	payLoadPtr[i++] = frameLen>>8 &0xff;  
	payLoadPtr[i++] = frameLen&0xff;

	// NALU data   
	memcpy(&payLoadPtr[i],pVStream,frameLen);
	i+=frameLen;
	//send
	
	avPacket->m_nBodySize = i;
	avPacket->m_nTimeStamp = vTimeS;

	if(!RTMP_IsConnected(rtmp))
	{
	   RTMP_LogPrintf("Rtmp is not connect\n");
	   return -1;
	}

	if(!RTMP_SendPacket(rtmp,avPacket,0))
	{
	   RTMP_LogPrintf("Send Err\n");
	   return -1;
	}

    return 0;
}
int sendMetaData( RTMP *rtmp, RTMPPacket *avPacket, int duation_sec)
{

	typedef unsigned long long uInt64;

union av_intfloat64 {
	uInt64 i;
	double   f;
};

	union av_intfloat64 v;
	unsigned char *payLoadPtr;
	unsigned int temp;
	int i;
	
    avPacket->m_hasAbsTimestamp = 0;
    avPacket->m_nChannel = AV_CHANNEL; 
    avPacket->m_nInfoField2 =1;
	avPacket->m_headerType = RTMP_PACKET_SIZE_LARGE;
	payLoadPtr = (unsigned char*)avPacket->m_body;

	

	v.f = duation_sec;

	i = 0;

	payLoadPtr[i++] = 0x2;
	payLoadPtr[i++] = 0;
	payLoadPtr[i++] = 10;
	strcpy((char*)payLoadPtr+i, "onMetaData");	
	i+=10;

	payLoadPtr[i++] = 0x08;
	payLoadPtr[i++] = 0x00;
	payLoadPtr[i++] = 0x00;
	payLoadPtr[i++] = 0x00;	
	payLoadPtr[i++] = 0x01; //1 duration 

	payLoadPtr[i++] = 0;
	payLoadPtr[i++] = 0x08;
	strcpy((char*)payLoadPtr+i, "duration");	
	i+=8;
	//put double 
	temp  = (unsigned int)(v.i>>32);
	payLoadPtr[i++] = 0;
	payLoadPtr[i++] = (unsigned char)(temp>>24);
	payLoadPtr[i++] = (unsigned char)((temp>>16)&0xff);
	payLoadPtr[i++] = (unsigned char)((temp>>8)&0xff);
	payLoadPtr[i++] = (unsigned char)(temp&0xff);
	temp  = (unsigned int)(v.i&0xffffffff);
	payLoadPtr[i++] = (unsigned char)((temp>>24)&0xff);
	payLoadPtr[i++] = (unsigned char)((temp>>16)&0xff);
	payLoadPtr[i++] = (unsigned char)((temp>>8)&0xff);
	payLoadPtr[i++] = (unsigned char)(temp&0xff);

	payLoadPtr[i++] = 0;//end
	payLoadPtr[i++] = 0;//end
	payLoadPtr[i++] = 9;//end

	avPacket->m_nBodySize = i;
	avPacket->m_nTimeStamp = 0;
	avPacket->m_packetType = RTMP_PACKET_TYPE_INFO;

	if(!RTMP_IsConnected(rtmp))
	{
		RTMP_LogPrintf("Rtmp is not connect\n");
		return -1;
	}

	if(!RTMP_SendPacket(rtmp,avPacket,0))
	{
		RTMP_LogPrintf("Send Err\n");
		return -1;
	}
	return 0;
}

#define PTS_DELAY_MS_200 200
#define PTS_DELAY_MS_100 100

TFTYPE playLiveStreamSending2(STREAMING_SERVER *server, const unsigned char* video_data, const int video_len, int FrameType, int rate)
{
	int frameLen, nalType, i,  frequencyBits;
	unsigned int  fpsV_ms, fpsA_ms, samplerate, nChannels, audioType;
	RTMP *rtmp;
    RTMPPacket *avPacket;
	unsigned char  *pVStream = NULL;
	unsigned char *payLoadPtr;
	int64 waitMs;

    frameLen = video_len;
	rtmp = server->rtmpSession;
	if(0xff==FrameType)
	{
		//!数据已播放完
		RTMP_SendCtrl(rtmp, 1, AV_CHANNEL, 0); //steam eof
		SendPlayStop(rtmp);
		
		//mutex_lock(server->mutexSend);
		//server->isEof = 1;		
		//mutex_unlock(server->mutexSend);	
		
		goto ERR_EXIT;
	}
	
    waitMs = server->startMs - Milliseconds();
	if(waitMs>1000*30)
		waitMs = 1000;
	
	if(waitMs > 0 && FrameType<3 && 2==server->playType)
	{
       //wait
       usleep(waitMs*1000);
       //waitRelative(waitMs, &server->condTimer, &server->mutexTimer);
	}
	
    avPacket = &server->avPacket;
    avPacket->m_hasAbsTimestamp = 0;
    avPacket->m_nChannel = AV_CHANNEL; 
    avPacket->m_nInfoField2 =1;
	avPacket->m_headerType = RTMP_PACKET_SIZE_LARGE;
	payLoadPtr = (unsigned char*)avPacket->m_body;

	if(server->aTimeS)
	{
		if(server->aTimeS > server->vTimeS+PTS_DELAY_MS_200)
			server->vTimeS = server->aTimeS;
		else if(server->aTimeS+PTS_DELAY_MS_200 < server->vTimeS)
			rate<<=3;
		else if(server->aTimeS+PTS_DELAY_MS_100 < server->vTimeS)
			rate<<=2;
	}

	//not enough space for whole av frame
    if(frameLen > AV_PACKET_SIZE)
    {
		server->sendSps = 0;
		return 0;
    }
	
	//fp = server->fp;
	frameLen = video_len;

	if(server->pause)
	{
		TFRET();
	}
								
	if(1==FrameType)//idr avc frame
	{			 
        int nalLen;
		
		fpsV_ms =  1000/rate;	
		avPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
		server->startMs+=fpsV_ms;

        nalType = AIPU_SPS;
        if(readNALUFromIDR((unsigned char*)video_data, frameLen, nalType, &nalLen, &pVStream)<0)
        {
			RTMP_Log(RTMP_LOGERROR, "Cann't find SPS!\n");
			goto ERR_EXIT;
		}

		if(!server->sendSps)
		{
			server->sendSps = 1;
			server->vTimeS = 0;
		}
		else
			server->vTimeS += fpsV_ms;	
	
		i=0;
		payLoadPtr[i++] = 0x17;
		payLoadPtr[i++] = 0x00;

		payLoadPtr[i++] = 0x00;
		payLoadPtr[i++] = 0x00;
		payLoadPtr[i++] = 0x00;

		//AVCDecoderConfigurationRecord
		payLoadPtr[i++] = 0x01;
		payLoadPtr[i++] = pVStream[1]; //profileId
		payLoadPtr[i++] = pVStream[2]; //profileCompat
		payLoadPtr[i++] = pVStream[3]; //levelId
		payLoadPtr[i++] = 0xff;

		//sps
		payLoadPtr[i++]   = 0xe1;
		payLoadPtr[i++] = (nalLen >> 8) & 0xff;
		payLoadPtr[i++] = nalLen & 0xff;

		memcpy(&payLoadPtr[i],pVStream,nalLen);
		i +=  nalLen;

		nalType = AIPU_PPS;
		if(readNALUFromIDR((unsigned char*)video_data, frameLen, nalType, &nalLen, &pVStream)<0)
		{
			RTMP_Log(RTMP_LOGERROR, "Cann't find PPS!\n");
			goto ERR_EXIT;
		}
        
		payLoadPtr[i++]   = 0x01;
		payLoadPtr[i++] = (nalLen >> 8) & 0xff;
		payLoadPtr[i++] = (nalLen) & 0xff;
		memcpy(&payLoadPtr[i],pVStream,nalLen);
		i +=  nalLen;
		
		//send
		avPacket->m_nBodySize = i;
		avPacket->m_nTimeStamp = server->vTimeS;

		if(!RTMP_IsConnected(rtmp))
		{
		   RTMP_LogPrintf("Rtmp is not connect\n");
		   goto ERR_EXIT;
		}

		if(!RTMP_SendPacket(rtmp,avPacket,0))
		{
		   RTMP_LogPrintf("Send Err\n");
		   goto ERR_EXIT;
		}

		nalType = AIPU_SEI;
		if(!readNALUFromIDR((unsigned char*)video_data, frameLen, nalType, &nalLen, &pVStream))
		{			  
		   //sei
		   if(sendPacket(avPacket, rtmp, 0x27, server->vTimeS, pVStream, nalLen)<0)
		   {
			   goto ERR_EXIT;
		   }
		}

		//idr
		nalType = AIPU_IDR;
		if(readNALUFromIDR((unsigned char*)video_data, frameLen, nalType, &nalLen, &pVStream)<0)
		{
			RTMP_Log(RTMP_LOGERROR, "Cann't find SPS!\n");
			goto ERR_EXIT;
		}

		if(sendPacket(avPacket, rtmp, 0x17, server->vTimeS, pVStream, nalLen)<0)
		{
		   goto ERR_EXIT;
		}
	}
	else if(0==FrameType && server->sendSps)
	{
		int skipLen;
		fpsV_ms =  1000/rate;
		server->vTimeS += fpsV_ms;
		avPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;  
		server->startMs+=fpsV_ms;
		
		if(!video_data[0] 
			&& !video_data[1] 
			&& !video_data[2] 
		&& 1==video_data[3] && AIPU_NON_IDR==(video_data[4]&0x1f))  
		{  
			skipLen = 4;
		}
		else if( !video_data[0] && 
			!video_data[1] 
		&& (1==video_data[2]) 
		&& (AIPU_NON_IDR==(video_data[3]&0x1f) )
		)
		{
			skipLen = 3;
		}
		else
			goto ERR_EXIT;

        
		if(sendPacket(avPacket, rtmp, 0x27, server->vTimeS, (unsigned char*)video_data+skipLen, frameLen-skipLen)<0)
		{
			goto ERR_EXIT;
		}

     }
	 else if(3==FrameType)
	 {	    
		avPacket->m_packetType = RTMP_PACKET_TYPE_AUDIO;
		audioType = 1 + (video_data[2]>>6);
		frequencyBits = (video_data[2]&0x3c)>>2;
		nChannels = ((video_data[2]&0x1)<<2) | (video_data[3]>>6);
		if(7==nChannels)
		 nChannels = 8;

		samplerate = adts_sample_rates[frequencyBits];
		server->bSendAudioCfg = server->sampleRate==samplerate ? 1:0;

		if(samplerate)
			 fpsA_ms = 1024*1000/samplerate;
		else
		{			 
			RTMP_LogPrintf("samplerate is 0\n");
			goto ERR_EXIT;
		}
		
		if(!server->bSendAudioCfg)
		{
			i = 0;
			payLoadPtr[i++] = 0xaf;  //aac
			payLoadPtr[i++] = 0x00; //AAC sequence header
			//audioObjectType 5 bits |samplingFrequencyIndex 4 bits | channelConfiguration 4 bits | 000  
			payLoadPtr[i++] = (audioType<<3)|(frequencyBits>>1);//aac-lc 2
			payLoadPtr[i++] = (frequencyBits<<7)|(nChannels<<3);

			avPacket->m_nBodySize = i;
			avPacket->m_nTimeStamp = server->aTimeS;

			if(!RTMP_IsConnected(rtmp))
			{
				RTMP_LogPrintf("Rtmp is not connect\n");
				goto ERR_EXIT;
			}

			if(!RTMP_SendPacket(rtmp,avPacket,0) )
			{
				RTMP_LogPrintf("Send Err\n");
				goto ERR_EXIT;
			}
			server->bSendAudioCfg = 1;
			server->sampleRate = samplerate;
		}

		i = 0;
		payLoadPtr[i++] = 0xaf;  //aac
		payLoadPtr[i++] = 0x01; //AAC RAW

		//Copy Raw AAC frame data
		memcpy(&payLoadPtr[i], video_data+7, frameLen-7);
		i+=(frameLen-7);
		avPacket->m_nBodySize = i;
		avPacket->m_nTimeStamp = server->aTimeS;
		if(server->aTimeS)
			server->aTimeS += fpsA_ms;
		else 
			server->aTimeS = server->vTimeS;

		if(!RTMP_IsConnected(rtmp))
		{
			RTMP_LogPrintf("Rtmp is not connect\n");
			goto ERR_EXIT;
		}

		if(!RTMP_SendPacket(rtmp,avPacket,0))
		{
			RTMP_LogPrintf("Send Err\n");
			goto ERR_EXIT;
		}

	 }
	 else
	 {
		return 0;
	 }
	
ERR_EXIT:		
	//mutex_lock(server->mutexSend);
	//server->startSessoin = 0;	
	//mutex_unlock(server->mutexSend);

	TFRET();
}

#if 0
TFTYPE playBackStreamSending(void *arg)
{
    FILE *fp;
	int pause=0;
	int frameLen, nalType, i, sendSps=0, FrequencyBits;
	unsigned int vTimeS=0, fpsV_ms = 0;
	//unsigned int windowSize;
	RTMP *rtmp;
    RTMPPacket *avPacket;
	STREAMING_SERVER * server = (STREAMING_SERVER *)arg;
	unsigned char *pInputVStream = NULL, *pVStream;
	flvContainer flvCfg={0};
	unsigned char *payLoadPtr;
	int64_t startMs;

	rtmp = server->rtmpSession;
    avPacket = &server->avPacket;
    avPacket->m_hasAbsTimestamp = 0;
    avPacket->m_nChannel = AV_CHANNEL; 
    avPacket->m_nInfoField2 =1;
	avPacket->m_headerType = RTMP_PACKET_SIZE_LARGE;
	payLoadPtr = (unsigned char*)avPacket->m_body;
	fp = server->fp;
	
	pInputVStream = (unsigned char *) malloc(VDECODER_BUFFER_SIZE);
    if (NULL == pInputVStream)
    {
		RTMP_Log(RTMP_LOGERROR, "malloc failed");
        goto ERR_EXIT;
    } 
	
	
	flvCfg.haveAudio = 1;
	flvCfg.haveVideo = 1;
	flvCfg.audioType = AIPU_AAC; //only support aac
	flvCfg.videoType = AIPU_H264;//only support h.264
    flvCfg.nChannels = 1;
	flvCfg.nSamplePerFrm = 1024;
	flvCfg.nSamplesPerSecond = 8000;
	flvCfg.nBitsPerSample = 16;
	flvCfg.frameRate = 25;
	
    switch(flvCfg.nSamplesPerSecond)
    {
        case 16000:
			FrequencyBits = 8;
		break;
		case 8000:
			FrequencyBits = 0xb;
		break;
		case 11025:
			FrequencyBits = 0xa;
		break;
		case 12000:
			FrequencyBits = 9;
		break;
		case 44100:
			FrequencyBits = 4;
		break;
		case 32000:
			FrequencyBits = 5;
		break;
		case 24000:
			FrequencyBits = 6;
		break;
		case 22050:
			FrequencyBits = 7;
		break;
		case 48000:
			FrequencyBits = 3;
		break;
		default:
			FrequencyBits = 4;
			break;
	}

    if(flvCfg.haveAudio)
    {
		if(16!=flvCfg.nBitsPerSample || AIPU_AAC!=flvCfg.audioType)
		{
			flvCfg.haveAudio = 0;
		}
	}

	
	if(flvCfg.haveVideo && AIPU_H264!=flvCfg.videoType)
	{
		RTMP_Log(RTMP_LOGERROR, "only support h.264");
        goto ERR_EXIT;

	}

	/*
	if(flvCfg.haveAudio)
	{  
		i = 0;
		payLoadPtr[i++] = 0xaf;  //aac
		payLoadPtr[i++] = 0x00; //AAC sequence header
		//audioObjectType 5 bits |samplingFrequencyIndex 4 bits | channelConfiguration 2 bits | 000  
		payLoadPtr[i++] = (2<<3)|(FrequencyBits>>1);//aac-lc 2
		payLoadPtr[i++] = (FrequencyBits<<7)|(flvCfg.nChannels<<3);

		avPacket->m_packetType = RTMP_PACKET_TYPE_AUDIO;
		avPacket->m_nBodySize = i;
		avPacket->m_nTimeStamp = aTimeS;
		fpsA_ms = (flvCfg.nSamplePerFrm*1000)/flvCfg.nSamplesPerSecond;

		if(!RTMP_IsConnected(rtmp))
		{
		   RTMP_LogPrintf("Rtmp is not connect\n");
		   goto ERR_EXIT;
		}

		if(!RTMP_SendPacket(rtmp,avPacket,0, 1))
		{
		   RTMP_LogPrintf("Send Err\n");
		   goto ERR_EXIT;
		}
		windowSize+=i;
	}
*/
    RTMP_LogPrintf("bufferInMs:%d ms\n",server->bufferInMs);
	startMs = clock()-server->bufferInMs;
	while(1)
	{
	 
	    mutex_lock(server->mutexSend);
	    if(server->pause)
	    {
			mutex_unlock(server->mutexSend);
			
			if(!pause)
			{			
				SendPauseResult(rtmp);
				pause = 1;
			}

			//windowSize = 0;
           // printf("wwwwwwwwwwwwwwwwwww hahaha\n");
			msleep(500);
			continue;
		}	
		mutex_unlock(server->mutexSend);

		if(pause)
			SendUnpauseResult(rtmp);		
		pause = 0;

		if((clock()-startMs)<vTimeS)
		{	
			// RTMP_LogPrintf("TimeStamp:%d ms\n",vTimeS);
			msleep(1);
			continue;
		}
				
		//**************just for sample*****************	
		//audio   
		/*	
		if(flvCfg.haveAudio)
		{  
			i = 0;
			payLoadPtr[i++] = 0xaf;  //aac
			payLoadPtr[i++] = 0x01; //AAC RAW
			
			aTimeS += fpsA_ms;

			//Copy Raw AAC frame data
			memcpy(&payLoadPtr[i],pAStream,frameLen);
			i+=frameLen
			avPacket->m_packetType = RTMP_PACKET_TYPE_AUDIO;
			avPacket->m_nBodySize = frameLen;
			avPacket->m_nTimeStamp = aTimeS;

			if(!RTMP_IsConnected(rtmp))
			{
			   RTMP_LogPrintf("Rtmp is not connect\n");
			   break;
			}

			if(!RTMP_SendPacket(rtmp,avPacket,0, 1))
			{
			   RTMP_LogPrintf("Send Err\n");
			   break;
			}

			//windowSize+=i;

            //if(!fpsV_ms)
				//msleep(fpsA_ms-2);
		}*/
			
		if(flvCfg.haveVideo)
		{
			if(readOneNALUFromEs(pInputVStream, &frameLen, fp, &nalType, &pVStream)<0)
			{
			    RTMP_LogPrintf("file read error\n");
				break;
			}
			
			if(frameLen > AV_PACKET_SIZE)
				sendSps = 0;
				
			if(AIPU_SPS==nalType)
			{			 
				//flvCfg.videoType = AIPU_H264;
				//**************just for sample:please set it from video cfg*****************	
				flvCfg.height = 704;
				flvCfg.width = 576;
				flvCfg.frameRate = 25;
				//*******************************************************************
				
				fpsV_ms =  1000/flvCfg.frameRate;
				
				if(!( AIPU_SPS==(pVStream[0]&0x1f)))  
				{  
					RTMP_Log(RTMP_LOGERROR, "Cann't find SPS!\n");
					break;
				} 
				
				//flvCfg.profileId = pVStream[5];
				//flvCfg.profileCompat = pVStream[6];
				//flvCfg.levelId = pVStream[7]; 
				i=0;
				payLoadPtr[i++] = 0x17;
				payLoadPtr[i++] = 0x00;
	
				payLoadPtr[i++] = 0x00;
				payLoadPtr[i++] = 0x00;
				payLoadPtr[i++] = 0x00;
	
				//AVCDecoderConfigurationRecord
				payLoadPtr[i++] = 0x01;
				payLoadPtr[i++] = pVStream[1]; //profileId
				payLoadPtr[i++] = pVStream[2]; //profileCompat
				payLoadPtr[i++] = pVStream[3]; //levelId
				payLoadPtr[i++] = 0xff;
	
				//sps
				payLoadPtr[i++]   = 0xe1;
				payLoadPtr[i++] = (frameLen >> 8) & 0xff;
				payLoadPtr[i++] = frameLen & 0xff;
	
				memcpy(&payLoadPtr[i],pVStream,frameLen);
				i +=  frameLen;
	
				if(readOneNALUFromEs(pInputVStream, &frameLen, fp, &nalType, &pVStream)<0)
				{
					RTMP_LogPrintf("file read error\n");
					break;
				}
				
				if(AIPU_PPS!=nalType)
					break;
	
				payLoadPtr[i++]   = 0x01;
				payLoadPtr[i++] = (frameLen >> 8) & 0xff;
				payLoadPtr[i++] = (frameLen) & 0xff;
				memcpy(&payLoadPtr[i],pVStream,frameLen);
				i +=  frameLen;
				//send
				avPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
				avPacket->m_nBodySize = i;
				avPacket->m_nTimeStamp = vTimeS;
	
				if(!RTMP_IsConnected(rtmp))
				{
				   RTMP_LogPrintf("Rtmp is not connect\n");
				   break;
				}
	
				if(!RTMP_SendPacket(rtmp,avPacket,0))
				{
				   RTMP_LogPrintf("Send Err\n");
				   break;
				}
				sendSps = 1;
	           // windowSize+= i;
			}
			else if(sendSps)
			{
				unsigned char frmType;
				if(AIPU_NON_IDR==nalType)
				{
					frmType = 0x27;
					vTimeS += fpsV_ms;
					//msleep(fpsV_ms-10);
				}
				else if(AIPU_SEI==nalType)
				{
					frmType = 0x27;
				}
				else if(AIPU_IDR==nalType )
				{
					frmType = 0x17;
					vTimeS += fpsV_ms;
					//msleep(fpsV_ms-10);
				}
				else
					break;
	
				i = 0;
				payLoadPtr[i++] = frmType;// 2:Pframe	7:AVC	
				payLoadPtr[i++] = 0x01;// AVC NALU	 
				payLoadPtr[i++] = 0x00;  
				payLoadPtr[i++] = 0x00;  
				payLoadPtr[i++] = 0x00;  
	
				// NALU size   
				payLoadPtr[i++] = frameLen>>24 &0xff;  
				payLoadPtr[i++] = frameLen>>16 &0xff;  
				payLoadPtr[i++] = frameLen>>8 &0xff;  
				payLoadPtr[i++] = frameLen&0xff;
	
				// NALU data   
				memcpy(&payLoadPtr[i],pVStream,frameLen);
	            i+=frameLen;
				//send
				avPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
				avPacket->m_nBodySize = i;
				avPacket->m_nTimeStamp = vTimeS;
	
				if(!RTMP_IsConnected(rtmp))
				{
				   RTMP_LogPrintf("Rtmp is not connect\n");
				   break;
				}
	
				if(!RTMP_SendPacket(rtmp,avPacket,0))
				{
				   RTMP_LogPrintf("Send Err\n");
				   break;
				}
				//windowSize+=i;
			}
		}
	}
		
	
ERR_EXIT:
		

	if(pInputVStream)
	{
		free(pInputVStream);
	}
	
	mutex_lock(server->mutexSend);
	server->startSessoin = 0;	
	mutex_unlock(server->mutexSend);

	TFRET();
}
#endif









