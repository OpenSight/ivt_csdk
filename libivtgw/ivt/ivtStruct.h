#include "ivtMacro.h"

#ifndef _IVTSTRUCT_
#define _IVTSTRUCT_

typedef struct
{
	int rpcType;
	int subType;
	void *params;
	int seq;
}ivtRPCStruct;

typedef struct
{
	int errCode;
	char msg[IVT_MSG_SIZE];
}ivtRPCErr;

//------------------------------rpc req---------------------------------------
typedef struct
{
	int channel;
	int qaulity;
	int max_bitrate;
	char url[IVT_URL_SIZE];
	char  streamID[IVT_STR_ID_SIZE];
}ivtRPCRtmpPublish;

typedef struct
{
	int channel;
	char streamID[IVT_STR_ID_SIZE];
}ivtRPCStopPublish;

typedef struct
{
	int channel;
}ivtRPCPTZPreList;

typedef struct
{
	int channel;
}ivtRPCPTZPreTList;//tourList

typedef struct
{
	int channel;
}ivtRPCRebootChl;//tourList

typedef struct
{
	char recSession[IVT_RCD_ID_SIZE];
	int channel;
	int qaulity;
	int seg_duration;
	int seg_max_size;
	int seg_max_count;
	int prerecord_seconds;
	int start_ts;
	int max_bitrate;
	char url[IVT_URL_SIZE];
}ivtRPCStartCR;

typedef struct
{
	char recSession[IVT_RCD_ID_SIZE];
	int channel;
}ivtRPCStopCR;

typedef struct
{
	int channel;
	int enable;
	int start;
	int end;
	int start1;
	int end1;
	int sensitivity;
	int delay;
}ivtRPCMDC;

typedef struct
{
	int channel;
	int enable;
	int start;
	int end;
	int start1;
	int end1;
	int sensitivity;
	int delay;
	int ulx;
	int uly;	
	int urx;
	int ury;	
	int dlx;
	int dly;
	int drx;
	int dry;
}ivtRPCRectDC;

//----------------------------ivt ptz event------------------------------------
typedef struct
{
	int  tNum;
	char token[IVT_TOKEN_NUM][IVT_PTZ_TOKEN_SIZE];
	char name[IVT_TOKEN_NUM][IVT_PTZ_NAME_SIZE];
}ivtRPCPTZPreListR;

typedef struct
{
    int  tNum;
	char token[IVT_TOKEN_NUM][IVT_PTZ_TOKEN_SIZE];
	char name[IVT_TOKEN_NUM][IVT_PTZ_NAME_SIZE];
}ivtRPCPTZPreTListR;//tourList

typedef struct
{
	int channel;
	int speed;
	int opType;
}ivtRPCPtzCtrl;

typedef struct
{
	int channel;
    char token[IVT_PTZ_TOKEN_SIZE];
}ivtRPCGotoPTZPreset;

typedef struct
{
	int channel;
    char token[IVT_PTZ_TOKEN_SIZE];
	int opType;
}ivtRPCCtrlPTZPresetTour;

typedef struct
{
	int channel;
	int opType;
}ivtRPCCtrlPTZPatrol;

typedef struct
{
	unsigned int dateTimeInSec;
	int offsetSec;
	int year;
	int mon;
	int day;
	int hour;
	int minute;
	int sec;
}ivtRPCSyncTime;
//---------------------------ivt-req-----------------------------------
typedef struct
{
	char url[IVT_URL_SIZE];
}ivcRPCPreviewRes;

typedef struct
{
	char version[IVT_VERSION_SIZE];
	char url[IVT_URL_SIZE];
	int forceToUpd;
}ivcRPCFirmwareRes;

typedef struct
{
	int state;
	int chnlState[IVT_CHANNEL_NUM];
	int chnlID[IVT_CHANNEL_NUM];
	char outAlarmState[IVT_CHANNEL_NUM];
	char motionState[IVT_CHANNEL_NUM];
	char rectState[IVT_CHANNEL_NUM]; //AlarmRectIntrusionDetectConfig
	char recSession[IVT_CHANNEL_NUM][IVT_RCD_ID_SIZE];
	int chnlNum;
}ivtRPCKeepAlive;

typedef struct
{
	int channel;
	int type;
	int state;	
	char desc[IVT_ALARM_DES_SIZE];
	
	char outAlarmState[IVT_CHANNEL_NUM];
	char motionState[IVT_CHANNEL_NUM];
	char rectState[IVT_CHANNEL_NUM]; //AlarmRectIntrusionDetectConfig
	//char lossState[IVT_CHANNEL_NUM];	
	//char blindState[IVT_CHANNEL_NUM];
	
}ivtRPCAlarmNotify;

typedef struct
{
	char url[IVT_URL_SIZE];
	int channel;
}ivcRPCAlarmRes;

//------------------------------------------------------------------------------
typedef int (*ivtCallBack)(ivtRPCStruct *para);
typedef int (*ivcCallBack)(ivtRPCStruct *para, ivtRPCStruct *retPara);

#endif






