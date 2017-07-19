#include "ivtMacro.h"

#ifndef _IVTCB_
#define _IVTCB_

#define HTTP_HOST_LEN 64
#define FM_STRING_LEN 32
extern char http_host[HTTP_HOST_LEN];
extern int http_port;
extern char ivtFirmware[FM_STRING_LEN];
extern ivcCallBack ivcReqCb[IVC_ELSE_METHOD];
extern ivtCallBack ivtEventCb[IVT_ELSE_EVENT];
extern ivtCallBack ivcEventCb[IVC_ELSE_EVENT];
extern ivtCallBack ivtResCb[IVT_ELSE_METHOD];
int getFirmAndMode(char *firmware, char *mode, int len);
int getUrl(char *code, char *pw, char *ip, int *port, int *enable, int *snap, int len);
int ivc_rtmpStopPublishAll();
int reboot(void);
int getAlarmPort(void);
int ivt_recvAlarmData(ivtRPCAlarmNotify *alarm, int fd);
int ivt_sendAlarmHeartBeat(int fd);
int ivc_stopCRCbAll();


#endif















































