#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "publishLive.h"
#include "log.h"
#define CHUNK_SIZE 4096
#define HTON32(x)  ((x>>24&0xff)|(x>>8&0xff00)|\
    (x<<8&0xff0000)|(x<<24&0xff000000))

#define AV_PACKET_SIZE (1024*(1024+512))
#define PTS_DELAY_MS_200 200
#define PTS_DELAY_MS_100 100

extern unsigned int adts_sample_rates[];

extern int readNALUFromIDR(unsigned char *pStream, int readSize, int naluType, 
                                 int *pFrameLen, unsigned char **pStreamStart);
extern int sendPacket(RTMPPacket *avPacket, RTMP *rtmp, int frmType, 
	                    int vTimeS, unsigned char *pVStream, int frameLen);

static int sendSetChunkSize1(RTMP* r)
{
    RTMPPacket packet;
    char pbuf[512]={0};//, *pend = pbuf+sizeof(pbuf);	
    char *enc;
    uint32_t chunkSize;
    packet.m_nChannel = 0x02;     // control channel (invoke)
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE; /* RTMP_PACKET_SIZE_MEDIUM; */
    packet.m_packetType = RTMP_PACKET_TYPE_CHUNK_SIZE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;	
    enc = packet.m_body;

	chunkSize=CHUNK_SIZE;
    chunkSize=HTON32(chunkSize);
    memcpy(enc,&chunkSize,4);
	r->m_outChunkSize = CHUNK_SIZE;

    packet.m_nBodySize =4;// enc - packet.m_body;	

    return RTMP_SendPacket(r, &packet, FALSE);

}


int publishLive_open(const char* url, rtmpPublisher *rPublish)
{
    RTMP *m_rtmp;

	if(!url)
		return -1;

	if(strlen(url)>512)
		return -1;
		
	m_rtmp = RTMP_Alloc();
	if(!m_rtmp)
		return -1;	
    rPublish->pRtmp = NULL;
	
	RTMP_Init(m_rtmp);

    m_rtmp->m_outChunkSize = CHUNK_SIZE;
	if(RTMP_SetupURL(m_rtmp,(char*)url) == FALSE)
	{
		RTMP_Free(m_rtmp);
		return -1;
	}

	RTMP_EnableWrite(m_rtmp);
	if (RTMP_Connect(m_rtmp, NULL) == FALSE) 
	{
		RTMP_Free(m_rtmp);
		return -1;
	} 
	
	sendSetChunkSize1(m_rtmp);
	
	if (RTMP_ConnectStream(m_rtmp,0) == FALSE)
	{
		RTMP_Close(m_rtmp);
		RTMP_Free(m_rtmp);
		return -1;
	}
        
	if(!RTMPPacket_Alloc(&rPublish->avPacket,AV_PACKET_SIZE+RTMP_MAX_HEADER_SIZE))
	{
		RTMP_Close(m_rtmp);
		RTMP_Free(m_rtmp);
		return -1;
	}

	rPublish->pRtmp = m_rtmp;
	rPublish->bSendAudioCfg = 0;
	rPublish->aTimeS = rPublish->vTimeS = 0;
	rPublish->sendSps = 0;
	rPublish->sampleRate = 0;
	strcpy(rPublish->url, url);
	
	return 0; 
}


int publishLive_sendAV(rtmpPublisher *rPublish, const unsigned char* videoData, const int frameLen, int frmType, int frmRate)
{
	int nalType, i,  frequencyBits;
	unsigned int  fpsV_ms, fpsA_ms, samplerate, nChannels, audioType;
	RTMP *rtmp;
    RTMPPacket *avPacket;
	unsigned char  *pVStream = NULL;
	unsigned char *payLoadPtr;

	if(0xff==frmType)
	{
		//there are some errors and no av data
		goto ERR_EXIT;
	}
	
	rtmp = rPublish->pRtmp;
	avPacket = &rPublish->avPacket;
	avPacket->m_hasAbsTimestamp = 0;
	avPacket->m_nChannel = 0x04; 
	avPacket->m_nInfoField2 =rtmp->m_stream_id;
	avPacket->m_headerType = RTMP_PACKET_SIZE_LARGE;
	payLoadPtr = (unsigned char*)avPacket->m_body;
    
	//not enough space for whole av frame
    if(frameLen > AV_PACKET_SIZE)
    {
		rPublish->sendSps = 0;
		return 0;
    }

	if(rPublish->aTimeS)
	{
		if(rPublish->aTimeS > rPublish->vTimeS+PTS_DELAY_MS_200)
			rPublish->vTimeS = rPublish->aTimeS;
		else if(rPublish->aTimeS+PTS_DELAY_MS_200 < rPublish->vTimeS)
			frmRate<<=3;
		else if(rPublish->aTimeS+PTS_DELAY_MS_100 < rPublish->vTimeS)
			frmRate<<=2;
	}

    if(1==frmType) //idr avc frame
    {
        int nalLen;
		fpsV_ms =  1000/frmRate;
		avPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
        avPacket->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
          
		//**************just for sample:please set it from video cfg*****************	
		nalType = AIPU_SPS;
		if(readNALUFromIDR((unsigned char*)videoData, frameLen, nalType, &nalLen, &pVStream)<0)
		{
			printf("Cann't find SPS!\n");
			goto ERR_EXIT;
		}

        if(!rPublish->sendSps)
		{
			rPublish->sendSps = 1;
			rPublish->vTimeS = 0;
		}
		else
			rPublish->vTimeS += fpsV_ms;				

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
		if(readNALUFromIDR((unsigned char*)videoData, frameLen, nalType, &nalLen, &pVStream)<0)
		{
			printf("Cann't find PPS!\n");
			goto ERR_EXIT;
		}


		payLoadPtr[i++]   = 0x01;
		payLoadPtr[i++] = (nalLen >> 8) & 0xff;
		payLoadPtr[i++] = (nalLen) & 0xff;
		memcpy(&payLoadPtr[i],pVStream,nalLen);
		i +=  nalLen;

		//send
		avPacket->m_nBodySize = i;
		avPacket->m_nTimeStamp = rPublish->vTimeS;

		if(!RTMP_IsConnected(rtmp))
		{
		   printf("Rtmp is not connect\n");
		   goto ERR_EXIT;
		}

		if(!RTMP_SendPacket(rtmp,avPacket,0))
		{
		   printf("Send Err\n");
		   goto ERR_EXIT;
		}

        /*
		nalType = AIPU_SEI;
		if(!readNALUFromIDR((unsigned char*)videoData, frameLen, nalType, &nalLen, &pVStream))
		{			  
		   //sei
		   if(sendPacket(avPacket, rtmp, 0x27, rPublish->vTimeS, pVStream, nalLen)<0)
		   {
			   goto ERR_EXIT;
		   }
		}
             */

		//idr
		avPacket->m_headerType = RTMP_PACKET_SIZE_LARGE;
		nalType = AIPU_IDR;
		if(readNALUFromIDR((unsigned char*)videoData, frameLen, nalType, &nalLen, &pVStream)<0)
		{
			printf("Cann't find SPS!\n");
			goto ERR_EXIT;
		}

		if(sendPacket(avPacket, rtmp, 0x17, rPublish->vTimeS, pVStream, nalLen)<0)
		{
		   goto ERR_EXIT;
		}

	}
	else if(0==frmType && rPublish->sendSps)
	{
	    int skipLen;
		fpsV_ms =  1000/frmRate;
		avPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
		
		rPublish->vTimeS += fpsV_ms;

		if(!videoData[0] 
			&& !videoData[1] 
			&& !videoData[2] 
		&& 1==videoData[3] && AIPU_NON_IDR==(videoData[4]&0x1f))  
		{  
			skipLen = 4;
		}
		else if( !videoData[0] && 
			!videoData[1] 
		&& (1==videoData[2]) 
		&& (AIPU_NON_IDR==(videoData[3]&0x1f) )
		)
		{
			skipLen = 3;
		}
		else
			goto ERR_EXIT;

		if(sendPacket(avPacket, rtmp, 0x27, rPublish->vTimeS, (unsigned char*)videoData+skipLen, frameLen-skipLen)<0)
		{
			goto ERR_EXIT;
		}
	}
	else if(3==frmType)
	{
	    /*
		audioType = AIPU_AAC; //only support aac
		nChannels = 2;
		nSamplePerFrm = 1024;
		nSamplesPerSecond = 16000;
		nBitsPerSample = 16;
		*/
		
		avPacket->m_packetType = RTMP_PACKET_TYPE_AUDIO;
		audioType = 1 + (videoData[2]>>6);
		frequencyBits = (videoData[2]&0x3c)>>2;
	    nChannels = ((videoData[2]&0x1)<<2) | (videoData[3]>>6);
		if(7==nChannels)
			nChannels = 8;
		
	    samplerate = adts_sample_rates[frequencyBits];
		rPublish->bSendAudioCfg = rPublish->sampleRate==samplerate ? 1:0;

		if(samplerate)
        	fpsA_ms = 1024*1000/samplerate;
		else
		{		    
			RTMP_LogPrintf("samplerate is 0\n");
			goto ERR_EXIT;
		}

		if(!rPublish->bSendAudioCfg)
		{
			i = 0;
			payLoadPtr[i++] = 0xaf;  //aac
			payLoadPtr[i++] = 0x00; //AAC sequence header
			//audioObjectType 5 bits |samplingFrequencyIndex 4 bits | channelConfiguration 4 bits | 000  
			payLoadPtr[i++] = (audioType<<3)|(frequencyBits>>1);//aac-lc 2
			payLoadPtr[i++] = (frequencyBits<<7)|(nChannels<<3);
			avPacket->m_nBodySize = i;
			avPacket->m_nTimeStamp = rPublish->aTimeS;

			if(!RTMP_IsConnected(rtmp))
			{
			   RTMP_LogPrintf("Rtmp is not connect\n");
			   goto ERR_EXIT;
			}

			if( !RTMP_SendPacket(rtmp,avPacket,0) )
			{
			   RTMP_LogPrintf("Send Err\n");
			   goto ERR_EXIT;
			}
			rPublish->sampleRate = samplerate;
			rPublish->bSendAudioCfg = 1;
		}
		
		i = 0;
		payLoadPtr[i++] = 0xaf;  //aac
		payLoadPtr[i++] = 0x01; //AAC RAW

		//Copy Raw AAC frame data
		memcpy(&payLoadPtr[i], videoData+7, frameLen-7);
		i+=(frameLen-7);
		avPacket->m_nBodySize = i;
		avPacket->m_nTimeStamp = rPublish->aTimeS;
		
		if(rPublish->aTimeS)
        	rPublish->aTimeS += fpsA_ms;
		else
			rPublish->aTimeS = rPublish->vTimeS;			
        
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

    return 0;
	
ERR_EXIT:
			
	return -1;
}

void publishLive_close(rtmpPublisher *rPublish)
{
	RTMP *pRtmp;

	pRtmp = rPublish->pRtmp;
	
	if(pRtmp)  
	{  
		RTMP_Close(pRtmp);  
		RTMP_Free(pRtmp);  
		rPublish->pRtmp = NULL;  
	}

	RTMPPacket_Free(&rPublish->avPacket);
}
















































