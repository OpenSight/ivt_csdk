#include "ivtRPC.h"
#include "string.h"
#include "ivtMacro.h"

int alarmPort;

ivtRPCWebSocket::ivtRPCWebSocket()
{
    int i;

	memset(m_url, 0, IVT_URL_SIZE);
	m_ctx = NULL;
	m_recv = NULL;
	m_send = NULL;
    m_doIvcReq = NULL;
    m_doIvcEvent = NULL;
	m_alarm = NULL;

	for(i=0;i<IVC_ELSE_EVENT;i++)
	{
		m_ivcEventCb[i] = NULL;
	}

	for(i=0;i<IVC_ELSE_METHOD;i++)
	{
		m_ivcReqCb[i] = NULL;
	}

	for(i=0;i<IVT_ELSE_EVENT;i++)
	{
		m_ivtEventCb[i] = NULL;
	}

	for(i=0;i<IVT_ELSE_METHOD;i++)
	{
		m_ivtReqCb[i] = NULL;
	}

}

ivtRPCWebSocket::~ivtRPCWebSocket()
{

}

int ivtRPCWebSocket::setIvcEventCallback(ivtCallBack cb, int eventType)
{
	if(eventType<IVC_ELSE_EVENT && eventType>-1)
		m_ivcEventCb[eventType] = cb;
	else
		return -1;

	return 0;
}

int ivtRPCWebSocket::setIvcReqCallback(ivcCallBack cb, int reqType)
{
	if(reqType<IVC_ELSE_METHOD && reqType>-1)
		m_ivcReqCb[reqType] = cb;
	else
		return -1;

	return 0;
}

int ivtRPCWebSocket::setIvtEventCallback(ivtCallBack cb, int eventType)
{
	if(eventType<IVT_ELSE_EVENT && eventType>-1)
		m_ivtEventCb[eventType] = cb;
	else
		return -1;

	return 0;
}

int ivtRPCWebSocket::setIvtReqCallback(ivtCallBack cb, int reqType)
{
	if(reqType<IVT_ELSE_METHOD && reqType>-1)
		m_ivtReqCb[reqType] = cb;
	else
		return -1;

	return 0;
}

int ivtRPCWebSocket::startRPC(char *wsUrl)
{
    int i;
    m_ctx = wsContextNew();
	if(!m_ctx)
	{
	    IVT_ERR("ERR\n");
		goto ERR;
	}

	if(!wsUrl)
	{	    
	    IVT_ERR("ERR\n");
		goto ERR;
	}

	if(strlen(wsUrl)>(IVT_URL_SIZE-1))
	{	    
	    IVT_ERR("ERR\n");
		goto ERR;
	}

	strcpy(m_url, wsUrl);
	if(wsCreateConnection(m_ctx,m_url)<0)
	{	    
	    IVT_ERR("ERR\n");
		goto ERR;
	}

	m_recv = new ivtRPCWebSocketRecv();
	m_send = new ivtRPCWebSocketSend();
	m_doIvcReq = new ivtDoIvcReq();
	m_doIvcEvent = new ivtDoIvcEvent();
	m_alarm = new ivtAlarmRecver();

	if(!m_recv || !m_send || !m_doIvcReq || !m_doIvcEvent || !m_alarm)
	{	    
	    IVT_ERR("ERR\n");
		goto ERR;
	}

   	for(i=0;i<IVC_ELSE_EVENT;i++)
	{
	    m_doIvcEvent->setIvcEventCallback(m_ivcEventCb[i],i);
	}

	for(i=0;i<IVC_ELSE_METHOD;i++)
	{
	    m_doIvcReq->setIvcReqCallback(m_ivcReqCb[i],i);
	}

	for(i=0;i<IVT_ELSE_EVENT;i++)
	{
		m_send->setIvtEventCallback(m_ivtEventCb[i], i);
	}

	for(i=0;i<IVT_ELSE_METHOD;i++)
	{
		m_send->setIvtReqCallback(m_ivtReqCb[i], i);
	}

	if(m_recv->startIvtRPCWebSocketRecv(m_ctx)<0)
	{		
		IVT_ERR("ERR\n");
		goto ERR;
	}

	if(m_alarm->startIvtAlarmRecver(alarmPort))
	{
		m_recv->stopIvtRPCWebSocketRecv();	
		IVT_ERR("ERR\n");
		goto ERR;
	}
	
	if(m_send->startIvtRPCWebSocketSend(m_ctx, m_recv, m_alarm)<0)
	{
	    m_recv->stopIvtRPCWebSocketRecv();			
	    m_alarm->stopIvtAlarmRecver();
		IVT_ERR("ERR\n");
		goto ERR;
	}

    if(m_doIvcReq->startDoIvcReq(m_ctx, m_recv)<0)
    {
		m_send->stopIvtRPCWebSocketSend();		
	    m_alarm->stopIvtAlarmRecver();
	    m_recv->stopIvtRPCWebSocketRecv();		
		IVT_ERR("ERR\n");
		goto ERR;
    }

    if(m_doIvcEvent->startDoIvcEvent(m_ctx, m_recv)<0)
    {
		m_doIvcReq->stopDoIvcReq();
		m_send->stopIvtRPCWebSocketSend();		
	    m_alarm->stopIvtAlarmRecver();
        m_recv->stopIvtRPCWebSocketRecv();		
		IVT_ERR("ERR\n");
		goto ERR;
    }

	return IVT_SUCCESS;
ERR:
	if(m_ctx)
	{
		wsContextFree(m_ctx);
		m_ctx = NULL;
	}

	if(m_recv)
		delete m_recv;

	if(m_send)
		delete m_send;

	if(m_doIvcReq)
		delete m_doIvcReq;

	if(m_doIvcEvent)
		delete m_doIvcEvent;

	 if(m_alarm)
	 	delete m_alarm;
	 
	m_alarm = NULL; 
	m_recv = NULL;
	m_send = NULL;
    m_doIvcReq = NULL;
    m_doIvcEvent = NULL;

	return IVT_FAILURE;
}

int ivtRPCWebSocket::stopRPC()
{
    //create thread here
	if(!m_recv || !m_send || !m_doIvcReq || !m_doIvcEvent || !m_alarm)
	{
		return IVT_FAILURE;
	}
	
    IVT_DEBUG("******dfdddddddddddddd****stopRPC**0*********\n");
	m_doIvcEvent->stopDoIvcEvent();
	
    IVT_DEBUG("******dfdddddddddddddd****stopRPC**1*********\n");
	m_doIvcReq->stopDoIvcReq();
	
    IVT_DEBUG("******dfdddddddddddddd****stopRPC**2*********\n");
	m_send->stopIvtRPCWebSocketSend();
	m_alarm->stopIvtAlarmRecver();
	
    IVT_DEBUG("******dfdddddddddddddd****stopRPC**3*********\n");
    m_recv->stopIvtRPCWebSocketRecv();
    IVT_DEBUG("******dfdddddddddddddd****stopRPC**4*********\n");

    if(m_ctx)
	{
		//sendCloseing(m_ctx, STATUS_NORMAL, "");
		wsContextFree(m_ctx);
		m_ctx = NULL;
	}

	delete m_recv;
	delete m_send;
	delete m_doIvcReq;
	delete m_doIvcEvent;
	delete m_alarm;
	m_recv = NULL;
	m_send = NULL;
    m_doIvcReq = NULL;
    m_doIvcEvent = NULL;
	m_alarm = NULL;
    IVT_DEBUG("******dfdddddddddddddd****stopRPC****5*******\n");

	return IVT_SUCCESS;
}







































