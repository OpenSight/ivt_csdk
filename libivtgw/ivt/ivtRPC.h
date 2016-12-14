#include "ivtRPCJson.h"
#include "ivtRecvSend.h"
#include "ivtMacro.h"
#include "../websocket/websocket.h"

#ifndef _IVTRPC_
#define _IVTRPC_

class ivtRPCWebSocket
{
	public:
		ivtRPCWebSocket();
		virtual ~ivtRPCWebSocket();

		int setIvcEventCallback(ivtCallBack cb, int eventType);//event form IVC
		int setIvcReqCallback(ivcCallBack cb, int reqType);  //req from IVC

		int setIvtEventCallback(ivtCallBack cb, int eventType); //event send from ivt
		int setIvtReqCallback(ivtCallBack cb, int reqType); //req send from ivt

		int startRPC(char *wsUrl);
		int stopRPC();
    public:		
		wsContext_t *m_ctx;
	private:
        char m_url[IVT_URL_SIZE];
        ivtRPCWebSocketRecv *m_recv;
		ivtRPCWebSocketSend *m_send;
		ivtDoIvcReq *m_doIvcReq;
		ivtDoIvcEvent *m_doIvcEvent;
		ivtAlarmRecver *m_alarm;

		ivtCallBack m_ivtEventCb[IVT_ELSE_EVENT];
		ivtCallBack m_ivcEventCb[IVC_ELSE_EVENT];
		ivtCallBack m_ivtReqCb[IVT_ELSE_METHOD];
		ivcCallBack m_ivcReqCb[IVC_ELSE_METHOD];
};


#endif







































