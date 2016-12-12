/*  Simple RTMP Server
 *  Copyright (C) 2009 Andrej Stepanchuk
 *  Copyright (C) 2009-2011 Howard Chu
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RTMPDump; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/* This is just a stub for an RTMP server. It doesn't do anything
 * beyond obtaining the connection parameters from the client.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "rtmp_sys.h"
#include "log.h"
#include "thread.h"
#include "rtmpsrv.h"
#include "sendFlv.h"
#include "stdio.h"

#include "Rtmp_vf.h"

static struct 	list_head 	server_list;
static MUTEX server_list_mutex;

struct 	list_head* GetServerList()
{
	return &server_list;
}

MUTEX* GetServerListMutex()
{
	return &server_list_mutex;
}

static int GetUrlLineData(char* strUrl, const char* strKey, char* strValue, int value_size)
{
    char* posF = 0;
    char* posQ = 0;

    memset(strValue, 0, value_size);
    posF = strstr(strUrl, strKey); 
    if (posF != NULL) 
    {
        posQ = strstr(posF, "&");
        if (posQ == NULL)
        {  
            posQ = strstr(posF, "/");  
            if (posQ == NULL) 
            {  
            	posQ = strUrl+strlen(strUrl);   
            }
        } 
		memcpy(strValue, posF+strlen(strKey), posQ-posF-strlen(strKey));

		return 0;
    }

	return -1;
}


//*********************************
volatile int sever_Run = 1;
//*********************************


//#define PACKET_SIZE 1024*1024

// server socket and state (our listening socket)
// client connection socket
#define HTON16(x)  ((x>>8&0xff)|(x<<8&0xff00))
#define HTON24(x)  ((x>>16&0xff)|(x<<16&0xff0000)|x&0xff00)
#define HTON32(x)  ((x>>24&0xff)|(x>>8&0xff00)|\
    (x<<8&0xff0000)|(x<<24&0xff000000))

//int startStreaming(const char *address, int port);
void AVreplace(AVal *src, const AVal *orig, const AVal *repl);

static const AVal av_dquote = AVC("\"");
static const AVal av_escdquote = AVC("\\\"");

#define STR2AVAL(av,str)	av.av_val = str; av.av_len = strlen(av.av_val)

/* this request is formed from the parameters and used to initialize a new request,
 * thus it is a default settings list. All settings can be overriden by specifying the
 * parameters in the GET request. */
//RTMP_REQUEST defaultRTMPRequest;

#define SAVC(x) static const AVal av_##x = AVC(#x)

SAVC(app);
SAVC(connect);
SAVC(flashVer);
SAVC(swfUrl);
SAVC(pageUrl);
SAVC(tcUrl);
SAVC(capabilities);
SAVC(audioCodecs);
SAVC(videoCodecs);
SAVC(videoFunction);
SAVC(objectEncoding);
SAVC(_result);
SAVC(createStream);
SAVC(getStreamLength);
SAVC(play);
SAVC(pause);
SAVC(fmsVer);
SAVC(mode);
SAVC(level);
SAVC(code);
SAVC(description);
SAVC(secureToken);

static int SendConnectResult(RTMP *r, double txn)
{
	RTMPPacket packet;
	char pbuf[384], *pend = pbuf+sizeof(pbuf);
	AMFObject obj;
	AMFObjectProperty p, op;
	AVal av;
	char *enc;
	packet.m_nChannel = 0x03;     // control channel (invoke)
	packet.m_headerType = 1; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 0;
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av__result);
	enc = AMF_EncodeNumber(enc, pend, txn);
	*enc++ = AMF_OBJECT;

	STR2AVAL(av, "FMS/3,5,1,525");
	enc = AMF_EncodeNamedString(enc, pend, &av_fmsVer, &av);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_capabilities, 31.0);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_mode, 1.0);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	*enc++ = AMF_OBJECT;

	STR2AVAL(av, "status");
	enc = AMF_EncodeNamedString(enc, pend, &av_level, &av);
	STR2AVAL(av, "NetConnection.Connect.Success");
	enc = AMF_EncodeNamedString(enc, pend, &av_code, &av);
	STR2AVAL(av, "Connection succeeded.");
	enc = AMF_EncodeNamedString(enc, pend, &av_description, &av);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_objectEncoding, r->m_fEncoding);
#if 0
	STR2AVAL(av, "58656322c972d6cdf2d776167575045f8484ea888e31c086f7b5ffbd0baec55ce442c2fb");
	enc = AMF_EncodeNamedString(enc, pend, &av_secureToken, &av);
#endif
	STR2AVAL(p.p_name, "version");
	STR2AVAL(p.p_vu.p_aval, "3,5,1,525");
	p.p_type = AMF_STRING;
	obj.o_num = 1;
	obj.o_props = &p;
	op.p_type = AMF_OBJECT;
	STR2AVAL(op.p_name, "data");
	op.p_vu.p_object = obj;
	enc = AMFProp_Encode(&op, enc, pend);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	packet.m_nBodySize = enc - packet.m_body;

	return RTMP_SendPacket(r, &packet, FALSE);
}

static int SendResultNumber(RTMP *r, double txn, double ID)
{
	RTMPPacket packet;
	char pbuf[256], *pend = pbuf+sizeof(pbuf);
	char *enc;

	packet.m_nChannel = 0x03;     // control channel (invoke)
	packet.m_headerType = 1; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 0;
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av__result);
	enc = AMF_EncodeNumber(enc, pend, txn);
	*enc++ = AMF_NULL;
	enc = AMF_EncodeNumber(enc, pend, ID);

	packet.m_nBodySize = enc - packet.m_body;

	return RTMP_SendPacket(r, &packet, FALSE);
}

SAVC(onStatus);
SAVC(status);
static const AVal av_NetStream_Play_Start = AVC("NetStream.Play.Start");
static const AVal av_Started_playing = AVC("Started playing");
static const AVal av_NetStream_Play_Stop = AVC("NetStream.Play.Stop");
static const AVal av_Stopped_playing = AVC("Stopped playing");
static const AVal av_Not_Found = AVC("AV File Not Found");
static const AVal av_NetStream_Play_StreamNotFound =AVC("NetStream.Play.StreamNotFound");
static const AVal av_NetStream_Play_Reset = AVC("NetStream.Play.Reset");
static const AVal av_NetStream_Play_Pause = AVC("NetStream.Pause.Notify");
static const AVal av_NetStream_Play_Unpause = AVC("NetStream.Unpause.Notify");

SAVC(details);
SAVC(clientid);
static const AVal av_NetStream_Authenticate_UsherToken = AVC("NetStream.Authenticate.UsherToken");

int SendUnpauseResult(RTMP *r)
{  
	RTMPPacket packet;
	char pbuf[512], *pend = pbuf+sizeof(pbuf);
	AVal av;
	char *enc;
	packet.m_nChannel = AV_CHANNEL;     // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 1; ////stream_id
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;	

	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av_onStatus);
	enc = AMF_EncodeNumber(enc, pend, 0);	
	*enc++=AMF_NULL;	
	*enc++ = AMF_OBJECT;	
	enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
	enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_Unpause);	
	STR2AVAL(av,"Unpause...");	
	enc = AMF_EncodeNamedString(enc, pend, &av_description, &av);
	enc = AMF_EncodeNamedString(enc, pend, &av_details, &r->Link.playpath);
	enc = AMF_EncodeNamedString(enc, pend, &av_clientid, &r->m_clientID);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;	
	packet.m_nBodySize = enc - packet.m_body;	
	return RTMP_SendPacket(r, &packet, FALSE);	
}

int SendPauseResult(RTMP*r)
{  
	RTMPPacket packet;
	char pbuf[512], *pend = pbuf+sizeof(pbuf);
	AVal av;
	char *enc;
	packet.m_nChannel = AV_CHANNEL;     // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 1; ////stream_id
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;	

	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av_onStatus);
	enc = AMF_EncodeNumber(enc, pend, 0);	
	*enc++=AMF_NULL;	
	*enc++ = AMF_OBJECT;	
	enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
	enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_Pause);	
	STR2AVAL(av,"pause...");	
	enc = AMF_EncodeNamedString(enc, pend, &av_description, &av);
	enc = AMF_EncodeNamedString(enc, pend, &av_details, &r->Link.playpath);
	enc = AMF_EncodeNamedString(enc, pend, &av_clientid, &r->m_clientID);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;	
	packet.m_nBodySize = enc - packet.m_body;	
	return RTMP_SendPacket(r, &packet, FALSE);	
}

static int SendPlayStart(RTMP *r)
{
    RTMPPacket packet;
    char pbuf[512], *pend = pbuf+sizeof(pbuf);
    AVal av;
    char *enc;
    packet.m_nChannel = AV_CHANNEL;     // control channel (invoke)
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE; /* RTMP_PACKET_SIZE_MEDIUM; */
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 1; //stream_id
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;	
    enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av_onStatus);
    enc = AMF_EncodeNumber(enc, pend, 0);
    *enc++=AMF_NULL;
    *enc++ = AMF_OBJECT;	
    enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
    enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_Start);
    STR2AVAL(av,"start...");
    enc = AMF_EncodeNamedString(enc, pend, &av_description, &av);
    enc = AMF_EncodeNamedString(enc, pend, &av_details, &r->Link.playpath);
    enc = AMF_EncodeNamedString(enc, pend, &av_clientid, &r->m_clientID);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;
    packet.m_nBodySize = enc - packet.m_body;
    return RTMP_SendPacket(r, &packet, FALSE);
}

static int SendPlayStreamNotFound(RTMP *r)
{
	RTMPPacket packet;
	char pbuf[512], *pend = pbuf+sizeof(pbuf);
	char *enc;

	packet.m_nChannel = AV_CHANNEL;     // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 1;//stream id
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av_onStatus);
	enc = AMF_EncodeNumber(enc, pend, 0);
	*enc++=AMF_NULL;
	*enc++ = AMF_OBJECT;

	enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
	enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_StreamNotFound);
	enc = AMF_EncodeNamedString(enc, pend, &av_description, &av_Not_Found);
	enc = AMF_EncodeNamedString(enc, pend, &av_details, &r->Link.playpath);
	enc = AMF_EncodeNamedString(enc, pend, &av_clientid, &r->m_clientID);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	packet.m_nBodySize = enc - packet.m_body;
	return RTMP_SendPacket(r, &packet, FALSE);
}

int SendPlayStop(RTMP *r)
{
	RTMPPacket packet;
	char pbuf[512], *pend = pbuf+sizeof(pbuf);
	char *enc;

	packet.m_nChannel = AV_CHANNEL;     // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 1;
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av_onStatus);
	enc = AMF_EncodeNumber(enc, pend, 0);
	*enc++=AMF_NULL;
	*enc++ = AMF_OBJECT;

	enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
	enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_Stop);
	enc = AMF_EncodeNamedString(enc, pend, &av_description, &av_Stopped_playing);
	enc = AMF_EncodeNamedString(enc, pend, &av_details, &r->Link.playpath);
	enc = AMF_EncodeNamedString(enc, pend, &av_clientid, &r->m_clientID);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	packet.m_nBodySize = enc - packet.m_body;
	return RTMP_SendPacket(r, &packet, FALSE);
}

static int SendResetResult(RTMP*r)
{  
	RTMPPacket packet;
	char pbuf[512], *pend = pbuf+sizeof(pbuf);
	AVal av;
	char *enc;
	packet.m_nChannel = AV_CHANNEL;     // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 1; //stream_id
	packet.m_hasAbsTimestamp = 0; //
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;	
	
	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av_onStatus);
	enc = AMF_EncodeNumber(enc, pend, 0);	
	*enc++=AMF_NULL;	
	*enc++ = AMF_OBJECT;	
	enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
	enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_Reset);	
	STR2AVAL(av,"reset...");	
	enc = AMF_EncodeNamedString(enc, pend, &av_description, &av);
	enc = AMF_EncodeNamedString(enc, pend, &av_details, &r->Link.playpath);
	enc = AMF_EncodeNamedString(enc, pend, &av_clientid, &r->m_clientID);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;	
    packet.m_nBodySize = enc - packet.m_body;	
	return RTMP_SendPacket(r, &packet, FALSE);	
}

static int SendSetCheunkSizeP2(RTMP*r)
{
	RTMPPacket packet;
	char pbuf[512]={0};//, *pend = pbuf+sizeof(pbuf);
	char *enc;
	packet.m_nChannel = 0x02;     // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_CONTROL;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 0;
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;	
	enc = packet.m_body;
	enc[5]=0x01;
    packet.m_nBodySize =6;// enc - packet.m_body;	
	return RTMP_SendPacket(r, &packet, FALSE);
}

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

static int SendRtmpSampleAccessResult(RTMP*r)
{
	RTMPPacket packet;
	char pbuf[512]={0}, *pend = pbuf+sizeof(pbuf);
	AVal av;
	char *enc;
	packet.m_nChannel = AV_CHANNEL; 
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet.m_packetType = 0x12;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 1; ////stream_id
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;	
	enc = packet.m_body;
	STR2AVAL(av,"|RtmpSampleAccess");
	enc = AMF_EncodeString(enc, pend, &av);
	*enc++ = 1;
	*enc++ = 0;
	*enc++ = 1;
	*enc++ = 0;
    packet.m_nBodySize =enc - packet.m_body;	
	return RTMP_SendPacket(r, &packet, FALSE);
}

int WindowAcknowledgementSize5(RTMP *r)
{
    RTMPPacket packet;
    char pbuf[512]={0};//, *pend = pbuf+sizeof(pbuf);	
    char *enc;
    uint32_t windsize;
    packet.m_nChannel = 0x02;     // control channel (invoke)
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE; /* RTMP_PACKET_SIZE_MEDIUM; */
    packet.m_packetType = RTMP_PACKET_TYPE_SERVER_BW;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;	
    enc = packet.m_body;
    windsize=WINDOW_SIZE;
    windsize=HTON32(windsize);
    memcpy(enc,&windsize,4);

    packet.m_nBodySize =4;// enc - packet.m_body;	

    return RTMP_SendPacket(r, &packet, FALSE);
}

int SendSetPeerBandWidth6(RTMP *r)
{
    RTMPPacket packet;
    char pbuf[512]={0};//, *pend = pbuf+sizeof(pbuf);
    char *enc;
    uint32_t windsize;
    packet.m_nChannel = 0x02;     // control channel (invoke)
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE; /* RTMP_PACKET_SIZE_MEDIUM; */
    packet.m_packetType = 0x06;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;	
    enc = packet.m_body;
    windsize=WINDOW_SIZE;
    windsize=HTON32(windsize);
    memcpy(enc,&windsize,4);
    enc[4]=0x02;
    packet.m_nBodySize =5;// enc - packet.m_body;

    return RTMP_SendPacket(r, &packet, FALSE);
}
static int SendSetCheunkSizeC3(RTMP *r)
{
    RTMPPacket packet;
    char pbuf[512]={0};//, *pend = pbuf+sizeof(pbuf);
    char *enc;
    packet.m_nChannel = 0x02;     // control channel (invoke)
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE; /* RTMP_PACKET_SIZE_MEDIUM; */
    packet.m_packetType = 0x04;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;	
    enc = packet.m_body;
    packet.m_nBodySize =6;// enc - packet.m_body;	

    return RTMP_SendPacket(r, &packet, FALSE);
}

static int SendonBWDoneResult(RTMP*r)
{
    RTMPPacket packet;
    char pbuf[512]={0}, *pend = pbuf+sizeof(pbuf);
    AVal av={0};
    char *enc;
    packet.m_nChannel = 0x03;
    packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM; //RTMP_PACKET_SIZE_LARGE
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 1;//stream_id
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;	
    enc = packet.m_body;
    STR2AVAL(av,"onBWDone");
    enc = AMF_EncodeString(enc, pend, &av);
    enc=AMF_EncodeNumber(enc,pend,0);
    *enc++=AMF_NULL;
    enc=AMF_EncodeNumber(enc,pend,0);
    packet.m_nBodySize =enc - packet.m_body;
    return RTMP_SendPacket(r, &packet, FALSE);
}

int ServeInvoke(STREAMING_SERVER *server, RTMP * r, 
                   RTMPPacket *packet, unsigned int offset)
{
	const char *body;
	unsigned int nBodySize;
	int ret = 0, nRes;
	AMFObject obj;
	AVal method;
	double txn;

	body = packet->m_body + offset;
	nBodySize = packet->m_nBodySize - offset;

	if (body[0] != 0x02)		// make sure it is a string method name we start with
	{
		RTMP_Log(RTMP_LOGWARNING, "%s, Sanity failed. no string method in invoke packet",
		__FUNCTION__);
		return 0;
	}

	nRes = AMF_Decode(&obj, body, nBodySize, FALSE);
	if (nRes < 0)
	{
		RTMP_Log(RTMP_LOGERROR, "%s, error decoding invoke packet", __FUNCTION__);
		return 0;
	}

	AMF_Dump(&obj);

	AMFProp_GetString(AMF_GetProp(&obj, NULL, 0), &method);
	txn = AMFProp_GetNumber(AMF_GetProp(&obj, NULL, 1));
	RTMP_Log(RTMP_LOGDEBUG, "%s, client invoking <%s>", __FUNCTION__, method.av_val);

	if (AVMATCH(&method, &av_connect))
	{
		AMFObject cobj;
		AVal pname, pval;
		int i;

		server->connect = packet->m_body;
		packet->m_body = NULL;

		AMFProp_GetObject(AMF_GetProp(&obj, NULL, 2), &cobj);
		for (i=0; i<cobj.o_num; i++)
		{
			pname = cobj.o_props[i].p_name;
			pval.av_val = NULL;
			pval.av_len = 0;
			if (cobj.o_props[i].p_type == AMF_STRING)
				pval = cobj.o_props[i].p_vu.p_aval;
			if (AVMATCH(&pname, &av_app))
			{
				r->Link.app = pval;
				pval.av_val = NULL;
				if (!r->Link.app.av_val)
					r->Link.app.av_val = "";
			}
			else if (AVMATCH(&pname, &av_flashVer))
			{
				r->Link.flashVer = pval;
				pval.av_val = NULL;
			}
			else if (AVMATCH(&pname, &av_swfUrl))
			{
				r->Link.swfUrl = pval;
				pval.av_val = NULL;
			}
			else if (AVMATCH(&pname, &av_tcUrl))
			{
				r->Link.tcUrl = pval;
				pval.av_val = NULL;
			}
			else if (AVMATCH(&pname, &av_pageUrl))
			{
				r->Link.pageUrl = pval;
				pval.av_val = NULL;
			}
			else if (AVMATCH(&pname, &av_audioCodecs))
			{
				r->m_fAudioCodecs = cobj.o_props[i].p_vu.p_number;
			}
			else if (AVMATCH(&pname, &av_videoCodecs))
			{
				r->m_fVideoCodecs = cobj.o_props[i].p_vu.p_number;
			}
			else if (AVMATCH(&pname, &av_objectEncoding))
			{
				r->m_fEncoding = cobj.o_props[i].p_vu.p_number;
			}
		}
		/* Still have more parameters? Copy them */
		if (obj.o_num > 3)
		{
			int i = obj.o_num - 3;
			r->Link.extras.o_num = i;
			r->Link.extras.o_props = malloc(i*sizeof(AMFObjectProperty));
			memcpy(r->Link.extras.o_props, obj.o_props+3, i*sizeof(AMFObjectProperty));
			obj.o_num = 3;
		}
		
	  	//SendConnectResult(r, txn);
	  	
        WindowAcknowledgementSize5(r);//set Window Acknowledgement Size 
        SendSetPeerBandWidth6(r);//set peer bandwidth
        SendSetCheunkSizeC3(r);//user control ping request
        SendConnectResult(r, txn);
        SendonBWDoneResult(r);//command message
	}
	else if(AVMATCH(&method, &av_createStream))
	{
	  	SendResultNumber(r, txn, 1);//streamID should set to 1
	}
	else if(AVMATCH(&method, &av_getStreamLength))
	{
	  	SendResultNumber(r, txn, 10.0);
	}
	else if(AVMATCH(&method, &av_NetStream_Authenticate_UsherToken))
	{
		AVal usherToken;
		AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &usherToken);
		AVreplace(&usherToken, &av_dquote, &av_escdquote);
		r->Link.usherToken = usherToken;
	}
	else if(AVMATCH(&method, &av_pause))
	{
	    int pause;
		pause = AMFProp_GetBoolean(AMF_GetProp(&obj, NULL, 3));
		mutex_lock(server->mutexSend);
		server->pause = pause;
		mutex_unlock(server->mutexSend);
		printf("pause %d\n", pause);

		//send 
		
	}
	else if(AVMATCH(&method, &av_play))
	{
		//char *cmd;
		//AVal *argv;
		//int  argc;
		//uint32_t now;
       // int resetFlag;
		RTMPPacket pc = {0};
		AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &r->Link.playpath);
		server->resetFlg= AMFProp_GetBoolean(AMF_GetProp(&obj, NULL, 6));//reset flag
		server->playType = 0; 
//			printf("--link hostname:%s, sockshost:%s,  playpath0:%s, playpath[%s:%d]-tcUrl:%s, swfUrl:%s, pageUrl:%s-app[%s:%d],auth:%s, flashVer:%s, 
//				subscribepath:%s, usherToken:%s, token:%s, pubUser:%s, pubPasswd:%s\n", 
//				r->Link.hostname.av_val, r->Link.sockshost.av_val, r->Link.playpath0.av_val, r->Link.playpath.av_val,r->Link.playpath.av_len
//				, r->Link.tcUrl.av_val, r->Link.swfUrl.av_val, r->Link.pageUrl.av_val
//				, r->Link.app.av_val, r->Link.app.av_len, r->Link.auth.av_val, r->Link.flashVer.av_val
//				, r->Link.subscribepath.av_val, r->Link.usherToken.av_val, r->Link.token.av_val
//				, r->Link.pubUser.av_val, r->Link.pubPasswd.av_val);
        if(r->Link.app.av_len)
        {
            if(strstr(r->Link.app.av_val, "live"))
            {
				char tmp[8] = {0};
				server->playType = 1;
				server->startPlay = 1;

				char* pval = NULL;
				if( strstr(r->Link.app.av_val, ".flv") )
				{
					pval = r->Link.app.av_val;
				}
				else if( strstr(r->Link.playpath.av_val, ".flv") )
				{
					pval = r->Link.playpath.av_val;
				}

				if( pval == NULL )
				{
					printf("no fit live path!n");
					printf("--link hostname:%s, sockshost:%s,  playpath0:%s, playpath[%s:%d]-tcUrl:%s, swfUrl:%s, pageUrl:%s-app[%s:%d],auth:%s, flashVer:%s, \
					subscribepath:%s, usherToken:%s, token:%s, pubUser:%s, pubPasswd:%s\n", 
					r->Link.hostname.av_val, r->Link.sockshost.av_val, r->Link.playpath0.av_val, r->Link.playpath.av_val,r->Link.playpath.av_len
					, r->Link.tcUrl.av_val, r->Link.swfUrl.av_val, r->Link.pageUrl.av_val
					, r->Link.app.av_val, r->Link.app.av_len, r->Link.auth.av_val, r->Link.flashVer.av_val
					, r->Link.subscribepath.av_val, r->Link.usherToken.av_val, r->Link.token.av_val
					, r->Link.pubUser.av_val, r->Link.pubPasswd.av_val);
					AMF_Reset(&obj);
					return 1;
				}

				//!分析通道，和码流类型
				///rtmp://172.16.2.14:1935/live/new/_channel_0_stream_0_.flv/
				char* p1 = strstr(pval, "_channel_");
				if(p1 != NULL)
				{
					p1 += strlen("_channel_");
					char* p2 = strstr(p1, "_");
					if(p2 != NULL)
					{ 
						memset(tmp, 0, 8);
						memcpy(tmp, p1, p2-p1);
						server->play_info.channel = atoi(tmp);
					}
				}
				p1 = strstr(pval, "_stream_");
				if(p1 != NULL)
				{
					p1 += strlen("_stream_");
					char* p2 = strstr(p1, "_");
					if(p2 != NULL)
					{
						memset(tmp, 0, 8);
						memcpy(tmp, p1, p2-p1);
						server->play_info.stream = atoi(tmp);
					}
				}

				if( server->play_info.channel < 0 
					|| server->play_info.stream < 0 )
				{
					printf("no fit channel[%d], stream[%d]\n", server->play_info.channel, server->play_info.stream);
					AMF_Reset(&obj);
					return 1;
				}
            }
            else if(strstr(r->Link.app.av_val, "playback"))
            {
				server->playType = 2;
				server->startPlay = 1;

				//"rtmp://172.16.2.14:1935/playback/new/id=%d&type=%d&pic=%d&Disk=%d&Part=%d&Clus=%d&record=%04d%02d%02d%02d%02d%02d_%04d%02d%02d%02d%02d%02d&pos=%s/"
				char* pval = NULL;
				pval = strstr(r->Link.app.av_val, "id=");
				if( pval == NULL  )
				{
					pval = strstr(r->Link.playpath.av_val, "id=");
				}

				if( pval == NULL )
				{
					printf("no fit playback path!n");
					printf("--link hostname:%s, sockshost:%s,  playpath0:%s, playpath[%s:%d]-tcUrl:%s, swfUrl:%s, pageUrl:%s-app[%s:%d],auth:%s, flashVer:%s, \
					subscribepath:%s, usherToken:%s, token:%s, pubUser:%s, pubPasswd:%s\n", 
					r->Link.hostname.av_val, r->Link.sockshost.av_val, r->Link.playpath0.av_val, r->Link.playpath.av_val,r->Link.playpath.av_len
					, r->Link.tcUrl.av_val, r->Link.swfUrl.av_val, r->Link.pageUrl.av_val
					, r->Link.app.av_val, r->Link.app.av_len, r->Link.auth.av_val, r->Link.flashVer.av_val
					, r->Link.subscribepath.av_val, r->Link.usherToken.av_val, r->Link.token.av_val
					, r->Link.pubUser.av_val, r->Link.pubPasswd.av_val);
					AMF_Reset(&obj);
					return 1;
				}

				char strType[128]= {0};
				GetUrlLineData(pval,"type=",strType, 128);
		        server->play_info.stream = atoi((char*)strType);
		        
		        GetUrlLineData(pval,"id=",strType, 128);
		        server->play_info.channel= atoi((char*)strType) - 1 ;

		        //GetUrlLineData(pval,"pic=",strType, 128);
		        //iPic= atoi((char*)strType.c_str());

		        GetUrlLineData(pval,"Disk=",strType, 128);
		        server->play_info.iDisk= atoi((char*)strType);
		        GetUrlLineData(pval,"Part=",strType, 128);
		        server->play_info.iPart= atoi((char*)strType);
		        GetUrlLineData(pval,"Clus=",strType, 128);
		        server->play_info.iCluster= atoi((char*)strType);	

				GetUrlLineData(pval,"record=",strType, 128);
				if( strlen(strType) > 0 )
				{
				    sscanf(strType, "%04d%02d%02d%02d%02d%02d_%04d%02d%02d%02d%02d%02d",
				        &server->play_info.st_year,
				        &server->play_info.st_month,
				        &server->play_info.st_day,
				        &server->play_info.st_hour,
				        &server->play_info.st_mintute,
				        &server->play_info.st_second,

				        &server->play_info.ed_year,
				        &server->play_info.ed_month,
				        &server->play_info.ed_day,
				        &server->play_info.ed_hour,
				        &server->play_info.ed_mintute,
				        &server->play_info.ed_second);
				}
				
            }			
        }

		if(2==server->playType)
		{
			unsigned char *p;
			unsigned int len = 0;
			if( (p=(unsigned char*)strstr(r->Link.app.av_val, "/") ) )
			{
                server->fileName[0] = '.';
                server->fileName[1] = '/';
                len = 2;
				strcpy(server->fileName+len, (char*)p+1);
				len = strlen(server->fileName);
                server->fileName[len] = '/';
                len+=1;
			}

            if(r->Link.playpath.av_len)
            {
				if('/'==*(r->Link.playpath.av_val+r->Link.playpath.av_len))
					r->Link.playpath.av_len -=1; 
				memcpy(server->fileName+len, r->Link.playpath.av_val, r->Link.playpath.av_len);
                *(server->fileName+len+r->Link.playpath.av_len) = '\0';
            }
			
		}
		pc.m_body = server->connect;
		server->connect = NULL;
		RTMPPacket_Free(&pc);
        RTMP_LogPrintf("***********:%s %d\n", server->fileName, server->playType);		
	}

	AMF_Reset(&obj);
	return ret;
}

static void HandleServerBW(RTMP *r, const RTMPPacket *packet)
{
  r->m_nServerBW = AMF_DecodeInt32(packet->m_body);
}

static void HandleClientBW(RTMP *r, const RTMPPacket *packet)
{
  r->m_nClientBW = AMF_DecodeInt32(packet->m_body);
  if (packet->m_nBodySize > 4)
    r->m_nClientBW2 = packet->m_body[4];
  else
    r->m_nClientBW2 = -1;
}

int ServePacket(STREAMING_SERVER *server, RTMP *r, RTMPPacket *packet)
{
	int ret = 0;
	int  nType;

	//RTMP_LogPrintf("received packet type %d, size %d bytes\n",
		//packet->m_packetType, packet->m_nBodySize);
    //server->readSize+=packet->m_nBodySize;
	
  	switch(packet->m_packetType)
    {
		case RTMP_PACKET_TYPE_CHUNK_SIZE:
		//HandleChangeChunkSize(r, packet);
		break;
		case RTMP_PACKET_TYPE_BYTES_READ_REPORT:
/*
			mutex_lock(server->mutexSend);
			server->winAck++;
			mutex_unlock(server->mutexSend); 
			size = AMF_DecodeInt32(packet->m_body);
			RTMP_LogPrintf("read BW = %d\n", size);
*/
		break;
		case RTMP_PACKET_TYPE_CONTROL:
			nType = 0;
			if (packet->m_body && packet->m_nBodySize >= 2)
				nType = AMF_DecodeInt16(packet->m_body);
             if(3==nType)			 
				 server->bufferInMs = AMF_DecodeInt32(packet->m_body + 2 +4);
		break;
		case RTMP_PACKET_TYPE_SERVER_BW:
			HandleServerBW(r, packet);
		break;
		case RTMP_PACKET_TYPE_CLIENT_BW:
			HandleClientBW(r, packet);
		break;
		case RTMP_PACKET_TYPE_AUDIO:
		break;
		case RTMP_PACKET_TYPE_VIDEO:
		break;
		case RTMP_PACKET_TYPE_FLEX_STREAM_SEND:
		break;
		case RTMP_PACKET_TYPE_FLEX_SHARED_OBJECT:
		break;
		case RTMP_PACKET_TYPE_FLEX_MESSAGE:
		{
			RTMP_Log(RTMP_LOGDEBUG, "%s, flex message, size %u bytes, not fully supported",
				__FUNCTION__, packet->m_nBodySize);

			if(ServeInvoke(server, r, packet, 1))
				RTMP_Close(r);
		break;
		}
		case RTMP_PACKET_TYPE_INFO:
		break;
		case RTMP_PACKET_TYPE_SHARED_OBJECT:
		break;
		case RTMP_PACKET_TYPE_INVOKE:
			RTMP_Log(RTMP_LOGDEBUG, "%s, received: invoke %u bytes", __FUNCTION__,
				packet->m_nBodySize);
			if(ServeInvoke(server, r, packet, 0))
				RTMP_Close(r);
		break;
		case RTMP_PACKET_TYPE_FLASH_VIDEO:
		break;
		default:
			RTMP_Log(RTMP_LOGDEBUG, "%s, unknown packet type received: 0x%02x", __FUNCTION__,
				packet->m_packetType);
#ifdef _DEBUG
		RTMP_LogHex(RTMP_LOGDEBUG, (unsigned char*)packet->m_body, packet->m_nBodySize);
#endif
    }
	return ret;
}


TFTYPE doServe(void *arg)
{
	fd_set fds;
	struct timeval tv;
	RTMP *rtmp = RTMP_Alloc(); //our session with the real client 
	RTMPPacket packet = { 0 };
	STREAMING_SERVER * server = (STREAMING_SERVER *)malloc(sizeof(STREAMING_SERVER));
	int sockfd = (int)arg;
	int ret;
	//int start;

	//timeout for http requests	
	pthread_detach(pthread_self());
	if(!server || !sockfd || !rtmp)
		goto quit;

	memset(server, 0, sizeof(STREAMING_SERVER));
	server->play_info.channel = -1;
	server->play_info.stream = -1;
    mutex_init(server->mutexSend);
	if(!RTMPPacket_Alloc(&server->avPacket,AV_PACKET_SIZE+RTMP_MAX_HEADER_SIZE))
		goto quit;
    
    RTMPPacket_Reset(&server->avPacket);
	
	memset(&tv, 0, sizeof(struct timeval));
	tv.tv_sec = 5;

	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);

	if(select(sockfd + 1, &fds, NULL, NULL, &tv) <= 0)
	{
		RTMP_Log(RTMP_LOGERROR, "Request timeout/select failed, ignoring request");
		goto quit;
	}
	else
	{
		RTMP_Init(rtmp);
		rtmp->m_sb.sb_socket = sockfd;
		sockfd = 0;
		/*
        if (sslCtx && !RTMP_TLS_Accept(rtmp, sslCtx))
		{
			RTMP_Log(RTMP_LOGERROR, "TLS handshake failed");
			goto cleanup;
		}*/
		if (!RTMP_Serve(rtmp))
		{
			RTMP_Log(RTMP_LOGERROR, "Handshake failed");
			goto cleanup;
		}
	}

	
	STR2AVAL(rtmp->Link.playpath,"28_18_86543210");
	while (RTMP_IsConnected(rtmp))
	{
		if(RTMP_ReadPacket(rtmp, &packet))
		{
			if (!RTMPPacket_IsReady(&packet))
				continue;
			ServePacket(server, rtmp, &packet);
			RTMPPacket_Free(&packet);

			if( !RTMP_IsConnected(rtmp) )
			{
				break;
			}
		}
		else
			break;
		
        if(server->startPlay && server->bufferInMs)
		{
			if(!server->startSessoin)
			{
			    //set  chunk Size
				SendSetCheunkSizeP2(rtmp);
				sendSetChunkSize1(rtmp);
				
				if(2==server->playType)
					RTMP_SendCtrl(rtmp, 4, AV_CHANNEL, 0);//stream is recorded  user control message
				
                RTMP_SendCtrl(rtmp, 0, AV_CHANNEL, 0);  //stream begin	
				
				if(server->resetFlg)
					SendResetResult(rtmp);  //onStatus play reset

				if(2==server->playType)
				{
					//rtmp://172.16.35.36:1935/playback/new/sample.flv/     ./new/sample.flv
					//server->fp =fopen(server->fileName,"rb");
					#if 0
					server->fp =fopen("ESV.data","rb");
					if(!server->fp)
					{
						SendPlayStreamNotFound(rtmp);
						server->startPlay = 0;
						break;
					}

					server->sendThread = ThreadCreate_rtmp(playBackStreamSending, server);
	                if(!server->sendThread)
	                {
						RTMP_SendCtrl(rtmp, 1, AV_CHANNEL, 0); //steam eof
						SendPlayStop(rtmp);
						break;
					}
					#endif

					mutex_lock(server_list_mutex);
					list_add(&server->server_list, GetServerList());
					mutex_unlock(server_list_mutex);

					RTMP_LogPrintf("bufferInMs:%d ms\n",server->bufferInMs);
					
					server->startMs = Milliseconds()-server->bufferInMs-BUF_DELAY_TUNE_MS;
					server->vTimeS = 0;
					server->aTimeS = 0;
					server->sendSps = 0;
					server->isEof = 0;
					server->rtmpSession = rtmp;
//						server->play_info.channel = 0;
//						server->play_info.stream = 0;
//						server->play_info.st_year = 2015;
//						server->play_info.st_month = 12;
//						server->play_info.st_day = 21;
//						server->play_info.st_hour = 14;
//						server->play_info.st_mintute = 00;
//						server->play_info.st_second = 0;
//						server->play_info.ed_year = 2015;
//						server->play_info.ed_month = 12;
//						server->play_info.ed_day = 21;
//						server->play_info.ed_hour = 17;
//						server->play_info.ed_mintute = 55;
//						server->play_info.ed_second = 0;

					SendPlayStart(rtmp);//onStatus play start
					SendRtmpSampleAccessResult(rtmp);

					int duration = (server->play_info.ed_hour-server->play_info.st_hour)*3600+
											(server->play_info.ed_mintute-server->play_info.st_mintute)*60+
											(server->play_info.ed_second-server->play_info.st_second);
					if(duration<0)
						duration += (24*3600);
					
					//RTMP_LogPrintf("duration:%d s\n",duration);
					sendMetaData(rtmp, &server->avPacket, duration);
				
					ret = RtmpGetLiveSource()->pfnOpenSource(&server->play_info, play_AddFrameData, &server->live_source_handle);
					if(ret < 0 )
					{
						SendPlayStreamNotFound(rtmp);
						server->startPlay = 0;
						break;
					}
					
				}
				else if(1==server->playType)
				{
					//!live直播
					mutex_lock(server_list_mutex);
					list_add(&server->server_list, GetServerList());
					mutex_unlock(server_list_mutex);

					RTMP_LogPrintf("bufferInMs:%d ms\n",server->bufferInMs);
					server->startMs = Milliseconds()-server->bufferInMs;
					server->vTimeS = 0;
					server->aTimeS = 0;
					server->sendSps = 0;
					server->isEof = 0;
					server->rtmpSession = rtmp;

					SendPlayStart(rtmp);//onStatus play start
					SendRtmpSampleAccessResult(rtmp);
					
					ret = RtmpGetLiveSource()->pfnOpenSource(&server->play_info, play_AddFrameData, &server->live_source_handle);
					if(ret < 0)
					{
						RTMP_SendCtrl(rtmp, 1, AV_CHANNEL, 0); //steam eof
						SendPlayStop(rtmp);
						server->startPlay = 0;
						break;
					}
				}

				//sendStream   playBackStreamSending
				server->startSessoin = 1;
				server->startPlay = 0;
			}
		}	
		/*
		mutex_lock(server->mutexSend);
		if(server->isEof)
		{		    
		mutex_unlock(server->mutexSend);
		break;
		}
		mutex_unlock(server->mutexSend);
		*/
	}
    
cleanup:

    mutex_lock(server_list_mutex);
    if(server->server_list.next != NULL 
        && server->server_list.prev != NULL
        )
	{
        list_del(&server->server_list);
    }
	mutex_unlock(server_list_mutex);
    
    if(NULL != server->live_source_handle)
	{
        ret = RtmpGetLiveSource()->pfnCloseSource(server->live_source_handle, &server->play_info);
        assert(ret == 0 );
		#if 0
        if(server->sendThread  > 0 )
        {
    		THREAD_JOIN(server->sendThread);
    	}
		#endif
    }
    
	RTMP_LogPrintf("Closing connection... ");
	RTMP_Close(rtmp);
	// Should probably be done by RTMP_Close() 
	rtmp->Link.playpath.av_val = NULL;
	rtmp->Link.tcUrl.av_val = NULL;
	rtmp->Link.swfUrl.av_val = NULL;
	rtmp->Link.pageUrl.av_val = NULL;
	rtmp->Link.app.av_val = NULL;
	rtmp->Link.flashVer.av_val = NULL;
	if (rtmp->Link.usherToken.av_val)
	{
		free(rtmp->Link.usherToken.av_val);
		rtmp->Link.usherToken.av_val = NULL;
	}

	RTMP_LogPrintf("done!\n\n");

quit:
	if(rtmp)
		RTMP_Free(rtmp);
	
	if(server)
	{
	    RTMPPacket_Free(&server->avPacket);
		
	    if(server->fp)
        	fclose(server->fp);
		
		mutex_destroy(server->mutexSend);
		free(server);
	}
	
	if(sockfd)
		closesocket(sockfd);

	TFRET();
}



TFTYPE serverRun(int socket)
{
	int listenFd = socket;
	int ret;
	fd_set fdset;
    struct timeval timeout;
	mutex_init(server_list_mutex);

	while(sever_Run)
	{
		FD_ZERO(&fdset);
	    FD_SET((unsigned int)listenFd, &fdset);
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;
		ret = select(listenFd + 1, &fdset, NULL, NULL, &timeout);
        
		if(ret>0)
		{
			struct sockaddr_in addr;
			socklen_t addrlen = sizeof(struct sockaddr_in);
			int sockfd = accept(listenFd, (struct sockaddr *) &addr, &addrlen);

			if (sockfd > 0)
			{
#ifdef linux
				//struct sockaddr_in dest;
				//char destch[16];
				//socklen_t destlen = sizeof(struct sockaddr_in);
				//getsockopt(sockfd, SOL_IP, SO_ORIGINAL_DST, &dest, &destlen);
				//strcpy(destch, inet_ntoa(dest.sin_addr));
				RTMP_Log(RTMP_LOGDEBUG, "%s: accepted connection from %s\n", __FUNCTION__,
				inet_ntoa(addr.sin_addr));
#else
				RTMP_Log(RTMP_LOGDEBUG, "%s: accepted connection from %s\n", __FUNCTION__,
				inet_ntoa(addr.sin_addr));
#endif
				// Create a new thread and transfer the control to that
				
			    ThreadCreate_rtmp(doServe, (void *)sockfd);
				//doServe(server, sockfd);
				RTMP_Log(RTMP_LOGDEBUG, "%s: processed request\n", __FUNCTION__);
			}
			else
			{
				RTMP_Log(RTMP_LOGERROR, "%s: accept failed", __FUNCTION__);
			}
		}
	}
	
	TFRET();
}

int startStreaming(const char *address, int port)
{
	struct sockaddr_in addr;
	int sockfd, tmp;
	//RTMP_SERVER *server;

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1)
	{
		RTMP_Log(RTMP_LOGERROR, "%s, couldn't create socket", __FUNCTION__);
		return 0;
	}

	tmp = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &tmp, sizeof(tmp));

	addr.sin_family = AF_INET;
	if(address)
		addr.sin_addr.s_addr = inet_addr(address);	//htonl(INADDR_ANY);
	else
		addr.sin_addr.s_addr = INADDR_ANY;
	
	addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1)
	{
		RTMP_Log(RTMP_LOGERROR, "%s, TCP bind failed for port number: %d", __FUNCTION__,
		           port);
		return 0;
	}

	if (listen(sockfd, 10) == -1)
	{
		RTMP_Log(RTMP_LOGERROR, "%s, listen failed", __FUNCTION__);
		closesocket(sockfd);
		return 0;
	}

	INIT_LIST_HEAD(GetServerList());
	
    serverRun(sockfd);
    closesocket(sockfd);
	return 1;
}

void AVreplace(AVal *src, const AVal *orig, const AVal *repl)
{
	char *srcbeg = src->av_val;
	char *srcend = src->av_val + src->av_len;
	char *dest, *sptr, *dptr;
	int n = 0;

    //count occurrences of orig in src 
	sptr = src->av_val;
	while (sptr < srcend && (sptr = strstr(sptr, orig->av_val)))
	{
		n++;
		sptr += orig->av_len;
	}
	if(!n)
		return;

	dest = malloc(src->av_len + 1 + (repl->av_len - orig->av_len) * n);
	sptr = src->av_val;
	dptr = dest;
	
	while (sptr < srcend && (sptr = strstr(sptr, orig->av_val)))
	{
		n = sptr - srcbeg;
		memcpy(dptr, srcbeg, n);
		dptr += n;
		memcpy(dptr, repl->av_val, repl->av_len);
		dptr += repl->av_len;
		sptr += orig->av_len;
		srcbeg = sptr;
	}
	n = srcend - srcbeg;
	memcpy(dptr, srcbeg, n);
	dptr += n;
	*dptr = '\0';
	src->av_val = dest;
	src->av_len = dptr - dest;
}


