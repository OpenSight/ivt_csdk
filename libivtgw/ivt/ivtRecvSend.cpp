#include "ivtRecvSend.h"
#include "ivtRPCJson.h"
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include "../http/httpClient.h"
#include "ivtCb.h"

//#include <errno.h>
//We do not use mutex in these two global variable.Bcz, RPC protocol can guarantee atom operation.
int ivt_req_gtype;
int ivt_send_req;
int timeOutS;
int firmwareSec;
int snap;

ivtRPCWebSocketRecv::ivtRPCWebSocketRecv():ivt_producerConsumerBase(),m_rpcJson()
{
	int i;

	pthread_mutex_init(&m_locker, NULL);
	m_ctx = NULL;
	memset(m_wsBuf, 0, WS_BUF_SIZE);

	for(i=0;i<ELEMENT_SIZE;i++)
    {
		m_reqData[i].params = NULL;
		m_eventData[i].params = NULL;
	}

	for(i=0;i<RES_ELEMENT_SIZE;i++)
		m_resData[i].params = NULL;

	m_structData.params = NULL;
}

ivtRPCWebSocketRecv::~ivtRPCWebSocketRecv()
{
	pthread_mutex_destroy(&m_locker);
}


int ivtRPCWebSocketRecv::startIvtRPCWebSocketRecv(wsContext_t *ctx)
{
    int i;

    m_ctx = ctx;
	m_rEvPos = 0;
	m_wEvPos = 0;
	m_rReqPos = 0;
	m_wReqPos = 0;
	m_rResPos = 0;
	m_wResPos = 0;
    m_dirtyEv = 0;
    m_dirtyReq = 0;
	m_dirtyRes = 0;

	sem_init(&m_semWEv, 0, ELEMENT_SIZE-1);
	sem_init(&m_semREv, 0, 0);
	sem_init(&m_semWReq, 0, ELEMENT_SIZE-1);
	sem_init(&m_semRReq, 0, 0);
	sem_init(&m_semWRes, 0, RES_ELEMENT_SIZE-1);
	sem_init(&m_semRRes, 0, 0);

	for(i=0;i<ELEMENT_SIZE;i++)
    {
		m_reqData[i].params = memalign(16, PARA_BUF_SIZE);
		m_eventData[i].params = memalign(16, PARA_BUF_SIZE);

		if((NULL==m_reqData[i].params) || (NULL==m_eventData[i].params))
		{
			IVT_ERR("memalign err!\n");
			stopIvtRPCWebSocketRecv();
			return -1;
		}
		memset(m_reqData[i].params, 0, PARA_BUF_SIZE);
		memset(m_eventData[i].params, 0, PARA_BUF_SIZE);
	}

	for(i=0;i<RES_ELEMENT_SIZE;i++)
	{
		m_resData[i].params = memalign(16, PARA_BUF_SIZE);
		if(NULL==m_resData[i].params)
		{
			IVT_ERR("memalign err!\n");
			stopIvtRPCWebSocketRecv();
			return -1;
		}
		memset(m_resData[i].params, 0, PARA_BUF_SIZE);
	}

	m_structData.params = memalign(16, PARA_BUF_SIZE);
	if(NULL == m_structData.params)
	{
		IVT_ERR("memalign err!\n");
		stopIvtRPCWebSocketRecv();
		return -1;
	}
	memset(m_structData.params, 0, PARA_BUF_SIZE);

	return startProducer();
}

int ivtRPCWebSocketRecv::stopIvtRPCWebSocketRecv()
{
	int i;

	pthread_mutex_lock(&m_killLocker);
	isKill = 1;
	pthread_mutex_unlock(&m_killLocker);
	sem_post(&m_semWEv);
	sem_post(&m_semWReq);
	sem_post(&m_semWRes);
	sendCloseing(m_ctx, STATUS_NORMAL, "");
    shutdown(m_ctx->fd,SHUT_RD);
    IVT_DEBUG("******dfdddddddddddddd****stopIvtRPCWebSocketRecv**3*********\n");
	stopProducer();
    IVT_DEBUG("******dfdddddddddddddd****stopIvtRPCWebSocketRecv**3*********\n");

	sem_destroy(&m_semWEv);
	sem_destroy(&m_semREv);
	sem_destroy(&m_semWReq);
	sem_destroy(&m_semRReq);
	sem_destroy(&m_semWRes);
	sem_destroy(&m_semRRes);

	for(i=0;i<ELEMENT_SIZE;i++)
    {
		if(m_reqData[i].params)
		{
			free(m_reqData[i].params);
			m_reqData[i].params = NULL;
		}

		if(m_eventData[i].params)
		{
			free(m_eventData[i].params);
			m_eventData[i].params = NULL;
		}
	}

	for(i=0;i<RES_ELEMENT_SIZE;i++)
	{
		if(m_resData[i].params)
		{
			free(m_resData[i].params);
			m_resData[i].params = NULL;
		}
	}

	if(m_structData.params)
	{
		free(m_structData.params);
		m_structData.params = NULL;
	}

	return IVT_SUCCESS;
}

void ivtRPCWebSocketRecv::producerTreadEntry()
{
	ivtRPCStruct *reqDst;
	ivtRPCStruct *resDst;
	ivtRPCStruct *evDst;

	while(1)
	{
	    pthread_mutex_lock(&m_killLocker);
		if(isKill)
		{
			pthread_mutex_unlock(&m_killLocker);
			goto ERR_EXIT;
		}
		pthread_mutex_unlock(&m_killLocker);

        //memset(m_wsBuf, 0, WS_BUF_SIZE);
		int len = recvData(m_ctx, m_wsBuf, WS_BUF_SIZE);
        if(len>0)
        {
            m_structData.subType = ivt_req_gtype;
			//IVT_DEBUG("recv %s\n", m_wsBuf);
			if(m_rpcJson.ivcPayLoadToStructData(m_wsBuf, &m_structData)<0)
				continue;

			switch(m_structData.rpcType)
			{
				case RPC_REQ:
				{
					sem_wait(&m_semWReq);
					reqDst = &m_reqData[m_wReqPos];
                    reqDst->rpcType = m_structData.rpcType;
					reqDst->subType = m_structData.subType;
					reqDst->seq = m_structData.seq;
					memcpy(reqDst->params, m_structData.params, PARA_BUF_SIZE);
					m_wReqPos++;
                    if(ELEMENT_SIZE==m_wReqPos)
						m_wReqPos = 0;

					sem_post(&m_semRReq);
				}
				break;
				case RPC_EVENT:
				{
					sem_wait(&m_semWEv);
					evDst = &m_eventData[m_wEvPos];
					evDst->rpcType = m_structData.rpcType;
					evDst->subType = m_structData.subType;
					evDst->seq = m_structData.seq;
					memcpy(evDst->params, m_structData.params, PARA_BUF_SIZE);
					m_wEvPos++;
					if(ELEMENT_SIZE==m_wEvPos)
						m_wEvPos = 0;
					sem_post(&m_semREv);
				}
				break;
				case RPC_ERR:
				case RPC_RESP:
				{
					//state machine
                    sem_wait(&m_semWRes);
					resDst = &m_resData[m_wResPos];
                    resDst->rpcType = m_structData.rpcType;
					resDst->subType = m_structData.subType;
					resDst->seq = m_structData.seq;
					memcpy(resDst->params, m_structData.params, PARA_BUF_SIZE);
					m_wResPos++;
                    if(RES_ELEMENT_SIZE==m_wResPos)
						m_wResPos = 0;

					sem_post(&m_semRRes);
				}
				break;
				default:
					break;
			}
		}
		else
		{
			//IVT_DEBUG("recvThreadFunc len is %d.\n", len);
			continue;
		}
	}
ERR_EXIT:
		pthread_exit(NULL);
}

int ivtRPCWebSocketRecv::consumerPendOut(int getType)
{
	if(RPC_EVENT==getType)
	{
	    pthread_mutex_lock(&m_locker);
	    m_dirtyEv = 1;
		pthread_mutex_unlock(&m_locker);
		sem_post(&m_semREv);
	}
	else if(RPC_REQ==getType)
	{
	    pthread_mutex_lock(&m_locker);
	    m_dirtyReq = 1;
	    pthread_mutex_unlock(&m_locker);
		sem_post(&m_semRReq);
	}
    else if(RPC_RESP==getType || RPC_ERR==getType)
    {
	    pthread_mutex_lock(&m_locker);
	    m_dirtyRes = 1;
	    pthread_mutex_unlock(&m_locker);
		sem_post(&m_semRRes);
	}
	else
		IVT_ERR("getType is unknown!\n");

	return IVT_SUCCESS;
}

int ivtRPCWebSocketRecv::consumerGetOneElement(void *element, int getType)
{
	ivtRPCStruct *reqDst;
	ivtRPCStruct *resDst;
	ivtRPCStruct *evDst;
    unsigned long *retVal;

	//this is important, think it twice more for the reason, maybe need a lock
	//Using m_dirty is more reasonable.
	retVal = (unsigned long *)element;

 	if(RPC_EVENT==getType)
 	{
		sem_wait(&m_semREv);

	    pthread_mutex_lock(&m_locker);
	    if(m_dirtyEv)
	    {
			*retVal=(unsigned long)NULL;
			pthread_mutex_unlock(&m_locker);
			return IVT_FAILURE;
	    }
		pthread_mutex_unlock(&m_locker);

		evDst = &m_eventData[m_rEvPos];
		*retVal = (unsigned long)evDst;
		m_rEvPos++;
		if(ELEMENT_SIZE==m_rEvPos)
			m_rEvPos = 0;
		sem_post(&m_semWEv);

	}
	else if(RPC_REQ==getType)
	{
		sem_wait(&m_semRReq);

	    pthread_mutex_lock(&m_locker);
		if(m_dirtyReq)
		{
			*retVal=(unsigned long)NULL;
			pthread_mutex_unlock(&m_locker);
			return IVT_FAILURE;
		}
		pthread_mutex_unlock(&m_locker);

		reqDst = &m_reqData[m_rReqPos];
		*retVal = (unsigned long)reqDst;
		m_rReqPos++;
		if(ELEMENT_SIZE==m_rReqPos)
			m_rReqPos = 0;
        sem_post(&m_semWReq);
	}
	else if(RPC_RESP==getType || RPC_ERR==getType)
	{
	    struct timespec ts;
		struct timeval t;
		int err;
		gettimeofday(&t, NULL);
		ts.tv_sec = t.tv_sec+20;
		ts.tv_nsec= t.tv_usec*1000;
        err=sem_timedwait(&m_semRRes, &ts);
		if(err<0)
		{
		    //close socket
            perror("sem_timedwait");
			return IVT_FAILURE;
		}

		pthread_mutex_lock(&m_locker);
		if(m_dirtyRes)
		{
			*retVal=(unsigned long)NULL;
			pthread_mutex_unlock(&m_locker);
			return IVT_FAILURE;
		}
		pthread_mutex_unlock(&m_locker);

		resDst = &m_resData[m_rResPos];
		*retVal = (unsigned long)resDst;
		m_rResPos++;
		if(RES_ELEMENT_SIZE==m_rResPos)
			m_rResPos = 0;
		sem_post(&m_semWRes);

	}
	else
	{
		IVT_ERR("getType is unknown!\n");
	}

    return IVT_SUCCESS;
}

//-------------------ivtRPCWebSocketSend----------------------------------
ivtRPCWebSocketSend::ivtRPCWebSocketSend():ivt_producerConsumerBase(),m_rpcJson()
{
	int i;

	m_resProducer = NULL;
	m_alarmProducer = NULL;
	for(i=0;i<(IVT_ELSE_METHOD+1);i++)
		m_ivtReqCb[i] = 0;

	for(i=0;i<(IVT_ELSE_EVENT+1);i++)
		m_ivtEventCb[i] = 0;

	m_wsBuf = NULL;
	//m_secCount = 0;
	m_ctx = NULL;
	m_reqSeq = 0;
}

ivtRPCWebSocketSend::~ivtRPCWebSocketSend()
{

}

int ivtRPCWebSocketSend::setIvtEventCallback(ivtCallBack cb, int eventType)
{
	if(eventType<IVT_ELSE_EVENT && eventType>-1)
		m_ivtEventCb[eventType] = cb;
	else
		return -1;

	return 0;
}

int ivtRPCWebSocketSend::setIvtReqCallback(ivtCallBack cb, int reqType)
{
	if(reqType<IVT_ELSE_METHOD && reqType>-1)
		m_ivtReqCb[reqType] = cb;
	else
		return -1;

	return 0;
}

int ivtRPCWebSocketSend::consumerGetOneElement(void *element, int getType)
{
	return IVT_SUCCESS;
}

int ivtRPCWebSocketSend::consumerPendOut(int getType)
{
	return IVT_SUCCESS;
}

int ivtRPCWebSocketSend::startIvtRPCWebSocketSend(wsContext_t *ctx,
	                          ivt_producerConsumerBase *producer, ivt_producerConsumerBase *alarmProducer)
{
    if(!ctx || !producer)
		return IVT_FAILURE;

	m_ctx = ctx;
	m_resProducer = producer;
	m_alarmProducer = alarmProducer;

	memset(m_outAlarmState, '0', IVT_CHANNEL_NUM);
	memset(m_motionState, '0', IVT_CHANNEL_NUM);
	
	m_wsBuf = (char *)memalign(16, WS_BUF_SIZE_IVT_REQ);
	if(NULL == m_wsBuf)
	{
		IVT_ERR("startDoIvcReq malloc err!\n");
		stopIvtRPCWebSocketSend();
		return IVT_FAILURE;
	}
	memset(m_wsBuf, 0, WS_BUF_SIZE_IVT_REQ);

	m_command.params = memalign(16, PARA_BUF_SIZE_IVT_REQ);
	if(NULL == m_command.params)
	{
		IVT_ERR("startIvtRPCWebSocketSend malloc err!\n");
		stopIvtRPCWebSocketSend();
		return IVT_FAILURE;
	}
	memset(m_command.params, 0, PARA_BUF_SIZE_IVT_REQ);

	
	return startProducer();
}

int ivtRPCWebSocketSend::stopIvtRPCWebSocketSend()
{
	pthread_mutex_lock(&m_killLocker);
	isKill = 1;
	pthread_mutex_unlock(&m_killLocker);
	m_alarmProducer->consumerPendOut(RPC_ELSE);
	m_resProducer->consumerPendOut(RPC_RESP);
	stopProducer();

	if(m_wsBuf)
	{
		free(m_wsBuf);
		m_wsBuf = NULL;
	}

	if(m_command.params)
	{
		free(m_command.params);
		m_command.params = NULL;
	}

	return IVT_SUCCESS;
}

int ivtRPCWebSocketSend::getHourSecTime(int &hour, int &sec)
{
    struct tm tm;
	struct timeval tv;
	time_t curTime;

	gettimeofday(&tv, NULL);
	curTime = tv.tv_sec;
	localtime_r(&curTime, &tm);

	hour = tm.tm_hour;
    sec = tm.tm_min*60+tm.tm_sec;
	return 0;
}

/*
int ivtRPCWebSocketSend::getSecTime()
{
    struct tm tm;
	struct timeval tv;
	time_t curTime;

	gettimeofday(&tv, NULL);
	curTime = tv.tv_sec;
	localtime_r(&curTime, &tm);

	return tm.tm_min*60+tm.tm_sec;
}
*/

int ivtRPCWebSocketSend::commandChoose()
{
	int cHour,cSec;
	struct timespec tsNow;

	while(1)
	{
		clock_gettime(CLOCK_MONOTONIC, &tsNow);

		getHourSecTime(cHour, cSec);
		if(FIRMWARE_HOUR!=cHour)
			m_firmWare = 1;

		m_cmdStruct = &m_command;
		if(IVC_UPGRADEFIRMWARE==ivt_send_req)
		{
			m_command.rpcType = RPC_REQ;
			m_command.subType = IVT_GET_FIRMWARE;
			m_command.seq = m_reqSeq;
			m_firmWare = 0;
			ivt_send_req = IVC_ELSE_EVENT;
			m_manFirmware = 1;
			break;
		}
		else if(FIRMWARE_HOUR==cHour && m_firmWare && firmwareSec<cSec) //&& !m_onVideo
		{
			m_command.rpcType = RPC_REQ;
			m_command.subType = IVT_GET_FIRMWARE;
			m_command.seq = m_reqSeq;
			m_firmWare = 0;
			m_manFirmware = 0;
			break;
		}
		else if(snap && (tsNow.tv_sec >= m_fTimePV+ IVT_SNAP_PEROID))
		{
			m_command.rpcType = RPC_REQ;
			m_command.subType = IVT_PREVIEW_SERVER;
			m_command.seq = m_reqSeq;
			m_fTimePV = tsNow.tv_sec;
			break;
		}
		else if(tsNow.tv_sec >=  m_fTimeKA + timeOutS)
		{
			m_command.rpcType = RPC_REQ;
			m_command.subType = IVT_KEEPALIVE;
			m_command.seq = m_reqSeq;
			m_fTimeKA = tsNow.tv_sec;
			break;
		}
		else
		{
		    ivtRPCAlarmNotify *alarmNofity=NULL;
		    m_cmdStruct = NULL;
			m_alarmProducer->consumerGetOneElement(&m_cmdStruct, RPC_ELSE);
	
			if(m_cmdStruct)
			{
				alarmNofity = (ivtRPCAlarmNotify *)m_cmdStruct->params;
				if(IVT_ALARM_HEARTBEAT==alarmNofity->type)
				{//copy states
					memcpy(m_outAlarmState, alarmNofity->outAlarmState, IVT_CHANNEL_NUM);
					memcpy(m_motionState, alarmNofity->motionState, IVT_CHANNEL_NUM);
					m_cmdStruct = NULL;
				}
				else
				{
					if(IVT_ALARM_MD==alarmNofity->type)
					{
						m_motionState[alarmNofity->channel] = alarmNofity->state ? '0':'1';
					}
					else if(IVT_ALARM_OUTSIDE==alarmNofity->type)
					{
						m_outAlarmState[alarmNofity->channel] = alarmNofity->state ? '0':'1';
					}
					m_cmdStruct->seq = m_reqSeq;
					break;
				}				
			}
			
			if(!m_cmdStruct)
			{
			    m_cmdStruct = &m_command;
				m_command.rpcType = RPC_REQ;
				m_command.subType = IVT_KEEPALIVE;
				m_command.seq = m_reqSeq;
				m_fTimeKA = tsNow.tv_sec;
				break;
			}
	
		}

	}

	return IVT_SUCCESS;
}

void ivtRPCWebSocketSend::producerTreadEntry()
{
    int len, i;
	int ret;
	//char pingString[100]= "aaaaaaaaaa yayayayay";
	ivtRPCStruct *resStruct;
	ivtRPCKeepAlive *keepAlive;
	struct timespec tsNow;

	m_cmdStruct=&m_command;
	clock_gettime(CLOCK_MONOTONIC, &tsNow);
	m_fTimePV  = tsNow.tv_sec;
    m_fTimeKA = 0;
	//m_secCount = 0;
	m_firmWare = 1;
	m_onVideo = 0;
	m_reqSeq = 0;
	m_manFirmware = 0;
	m_command.rpcType = RPC_REQ;
	m_command.subType = IVT_PREVIEW_SERVER;
    m_command.seq = 0;

    //sendPing(m_ctx, pingString, sizeof(pingString));
	while(1)
	{
	    pthread_mutex_lock(&m_killLocker);
		if(isKill)
		{
			pthread_mutex_unlock(&m_killLocker);
			goto ERR_EXIT;
		}
		pthread_mutex_unlock(&m_killLocker);

		switch(m_cmdStruct->rpcType)
		{
        	case RPC_REQ:
				ivt_req_gtype = m_cmdStruct->subType;
				if(IVT_KEEPALIVE==m_cmdStruct->subType)
				{
					if(m_ivtReqCb[IVT_KEEPALIVE])
				    	m_ivtReqCb[IVT_KEEPALIVE](m_cmdStruct);

					m_onVideo = 0;
					keepAlive = (ivtRPCKeepAlive *)m_cmdStruct->params;
                    
					memcpy(keepAlive->motionState, m_motionState, IVT_CHANNEL_NUM);
					memcpy(keepAlive->outAlarmState, m_outAlarmState, IVT_CHANNEL_NUM);
                    
					// In json format, keepAlive event must have a chnnl
					//if(!keepAlive->chnlNum)
					//	continue;
                        
					for(i=0;i<keepAlive->chnlNum;i++)
					{
					   if(IVT_CHNL_STATE_LIVE==keepAlive->chnlState[i])
					   {
						   m_onVideo = 1;
						   break;
					   }
					}
				}
			break;
			case RPC_EVENT:
			    if(m_ivtEventCb[m_cmdStruct->subType])
					m_ivtEventCb[m_cmdStruct->subType](m_cmdStruct);
			break;
			default:
			break;
		}
        
        //memset(m_wsBuf, 0, WS_BUF_SIZE_IVT_REQ);
		len = m_rpcJson.ivtStructDataToPayLoad(m_wsBuf, m_cmdStruct, WS_BUF_SIZE_IVT_REQ);
	    //IVT_DEBUG("send %s.\n", m_wsBuf);
		if(len > 0)
			ret = sendUtf8Data(m_ctx, m_wsBuf,len);
		else
			ret = IVT_FAILURE;

		if(IVT_FAILURE==ret)
		{
		    IVT_ERR("sendUtf8Data ret is %d.\n", ret);
			continue;
		}

		if(RPC_REQ==m_cmdStruct->rpcType)
		{
		   	if(m_resProducer->consumerGetOneElement(&resStruct, RPC_RESP)<0)
		   	{		   	
				//IVT_DEBUG("get socket err\n");
				mutex_lock(m_ctx->gMutex);
				m_ctx->socketErr = 1;
				mutex_unlock(m_ctx->gMutex);
				shutdown(m_ctx->fd,SHUT_RD);//SHUT_RDWR
				goto ERR_EXIT;
			}

			if(NULL==resStruct)
				continue;//do this req again

			if(m_reqSeq!=resStruct->seq)
				continue;//do this req again

            if(IVT_GET_FIRMWARE==resStruct->subType&&m_manFirmware)
            {
				ivcRPCFirmwareRes *res=(ivcRPCFirmwareRes *)(resStruct->params);
                res->forceToUpd = 1;
			}

			if(IVT_KEEPALIVE!=resStruct->subType && m_ivtReqCb[resStruct->subType])
			{
				if(IVT_ALARMNOTIFY==resStruct->subType)
				{
					ivcRPCAlarmRes *res=(ivcRPCAlarmRes *)(resStruct->params);
					ivtRPCAlarmNotify *req = (ivtRPCAlarmNotify *)(m_cmdStruct->params);
					res->channel = req->channel;
				}
				m_ivtReqCb[resStruct->subType](resStruct);
			}

			m_reqSeq++;
		}

        commandChoose();
        //command choose
	}

ERR_EXIT:
	pthread_exit(NULL);
}

//------------------------------------------------------------------------------------------
ivtDoIvcReq::ivtDoIvcReq():ivt_producerConsumerBase(),m_rpcJson()
{
	int i;
	m_ctx = NULL;

	for(i=0;i<(IVC_ELSE_METHOD+1);i++)
		m_ivcReqCb[i] = 0;

	m_wsBuf = NULL;
}

ivtDoIvcReq::~ivtDoIvcReq()
{

}

int ivtDoIvcReq::setIvcReqCallback(ivcCallBack cb, int reqType)
{
	if(reqType<IVC_ELSE_METHOD && reqType>-1)
		m_ivcReqCb[reqType] = cb;
	else
		return -1;

	return 0;
}

int ivtDoIvcReq::consumerGetOneElement(void *element, int getType)
{
	return IVT_SUCCESS;
}

int ivtDoIvcReq::consumerPendOut(int getType)
{
	return IVT_SUCCESS;
}

int ivtDoIvcReq::startDoIvcReq(wsContext_t *ctx, ivt_producerConsumerBase *producer)
{
	if(!ctx || !producer)
		return IVT_FAILURE;

	m_ctx = ctx;
	m_producer = producer;

	m_wsBuf = (char *)memalign(16, WS_BUF_SIZE_PTZ);
	if(NULL == m_wsBuf)
	{
		IVT_ERR("startDoIvcReq malloc err!\n");
		stopDoIvcReq();
		return IVT_FAILURE;
	}
	memset(m_wsBuf, 0, WS_BUF_SIZE_PTZ);

	m_res.params = memalign(16, PARA_BUF_SIZE_PTZ);
	if(NULL == m_res.params)
	{
		IVT_ERR("startDoIvcReq malloc err!\n");
		stopDoIvcReq();
		return IVT_FAILURE;
	}
	memset(m_res.params, 0, PARA_BUF_SIZE_PTZ);

	return startProducer();
}

int ivtDoIvcReq::stopDoIvcReq()
{
	pthread_mutex_lock(&m_killLocker);
	isKill = 1;
	pthread_mutex_unlock(&m_killLocker);
	m_producer->consumerPendOut(RPC_REQ);
	stopProducer();

	if(m_wsBuf)
	{
		free(m_wsBuf);
		m_wsBuf = NULL;
	}

	if(m_res.params)
	{
		free(m_res.params);
		m_res.params = NULL;
	}

	return IVT_SUCCESS;
}

void ivtDoIvcReq::producerTreadEntry()
{
	ivtRPCStruct *reqStruct;
	int len, ret;

	while(1)
	{
	    pthread_mutex_lock(&m_killLocker);
		if(isKill)
		{
			pthread_mutex_unlock(&m_killLocker);
			goto ERR_EXIT;
		}
		pthread_mutex_unlock(&m_killLocker);

		m_producer->consumerGetOneElement(&reqStruct, RPC_REQ);
		if(NULL==reqStruct)
			continue;

        //callback
		//memset(m_res.params, 0, PARA_BUF_SIZE_PTZ);//remove this line if ok
		if(IVC_ELSE_METHOD==reqStruct->subType)
		{
		    ivtRPCErr *err = (ivtRPCErr *)m_res.params;
			m_res.rpcType = RPC_ERR;
			m_res.subType = IVC_ELSE_METHOD;
			m_res.seq = reqStruct->seq;
		    err->errCode = 101;
	        strcpy(err->msg, "error");
			IVT_DEBUG("IVC_ELSE_METHOD!\n");
		}
		else if(IVC_REBOOTCHANNEL!=reqStruct->subType)
		{
			if(m_ivcReqCb[reqStruct->subType])
				m_ivcReqCb[reqStruct->subType](reqStruct, &m_res);
		}
		else
		{
			m_res.rpcType = RPC_RESP;
			m_res.subType = IVC_REBOOTCHANNEL;
			m_res.seq = reqStruct->seq;
		}
        //send res
        //memset(m_wsBuf, 0, WS_BUF_SIZE_PTZ);
        len = m_rpcJson.ivtStructDataToPayLoad(m_wsBuf, &m_res, WS_BUF_SIZE_PTZ);

		IVT_DEBUG("res m_wsBuf:%s!\n", m_wsBuf);
		if(len > 0)
			ret = sendUtf8Data(m_ctx, m_wsBuf,len);
		else
			ret = IVT_FAILURE;

		if(IVT_FAILURE==ret)
			continue;

        if(IVC_REBOOTCHANNEL==reqStruct->subType)
			m_ivcReqCb[IVC_REBOOTCHANNEL](reqStruct, &m_res);
	}

ERR_EXIT:
	pthread_exit(NULL);
}

//----------------------------------------------------------------------------

ivtDoIvcEvent::ivtDoIvcEvent():ivt_producerConsumerBase(),m_rpcJson()
{
	int i;
	m_ctx = NULL;

	for(i=0;i<(IVC_ELSE_EVENT+1);i++)
		m_ivcEventCb[i] = 0;
}

ivtDoIvcEvent::~ivtDoIvcEvent()
{

}

int ivtDoIvcEvent::setIvcEventCallback(ivtCallBack cb, int eventType)
{
	if(eventType<IVC_ELSE_EVENT&& eventType>-1)
		m_ivcEventCb[eventType] = cb;
	else
		return -1;

	return 0;
}

int ivtDoIvcEvent::consumerGetOneElement(void *element, int getType)
{
	return IVT_SUCCESS;
}

int ivtDoIvcEvent::consumerPendOut(int getType)
{
	return IVT_SUCCESS;
}

int ivtDoIvcEvent::startDoIvcEvent(wsContext_t *ctx, ivt_producerConsumerBase *producer)
{
	if(!ctx || !producer)
		return IVT_FAILURE;

	m_ctx = ctx;
	m_producer = producer;

	return startProducer();
}

int ivtDoIvcEvent::stopDoIvcEvent()
{
	pthread_mutex_lock(&m_killLocker);
	isKill = 1;
	pthread_mutex_unlock(&m_killLocker);
	m_producer->consumerPendOut(RPC_EVENT);

	stopProducer();

	return IVT_SUCCESS;
}

void ivtDoIvcEvent::producerTreadEntry()
{
	ivtRPCStruct *reqStruct;

	while(1)
	{
	    pthread_mutex_lock(&m_killLocker);
		if(isKill)
		{
			pthread_mutex_unlock(&m_killLocker);
			goto ERR_EXIT;
		}
		pthread_mutex_unlock(&m_killLocker);

		m_producer->consumerGetOneElement(&reqStruct, RPC_EVENT);
		if(NULL==reqStruct)
			continue;

        //callback
        if(IVC_UPGRADEFIRMWARE==reqStruct->subType)
			ivt_send_req = IVC_UPGRADEFIRMWARE;
        else if(m_ivcEventCb[reqStruct->subType])
			m_ivcEventCb[reqStruct->subType](reqStruct);
	}

ERR_EXIT:
	pthread_exit(NULL);
}

//--------------------------------------------------------------------------------------
ivtAlarmRecver::ivtAlarmRecver():ivt_producerConsumerBase()
{
	int i;

	pthread_mutex_init(&m_locker, NULL);
	m_alarmPort = -1;
	m_fd = -1;

	for(i=0;i<ELEMENT_SIZE;i++)
	{
		m_alarmData[i].params = NULL;
	}
	m_structData.params = NULL;
}

ivtAlarmRecver::~ivtAlarmRecver()
{
	pthread_mutex_destroy(&m_locker);
}

int ivtAlarmRecver::consumerGetOneElement(void *element, int getType)
{
    ivtRPCStruct *alarmDst;
	unsigned long *retVal;
    struct timespec ts;
    struct timeval t;
	int err;

	retVal = (unsigned long *)element;

	gettimeofday(&t, NULL);
	ts.tv_sec = t.tv_sec+10;
	ts.tv_nsec= t.tv_usec*1000;

	err=sem_timedwait(&m_semR, &ts);
	if(err<0)
	{
		return IVT_FAILURE;
	}

	pthread_mutex_lock(&m_locker);
	if(m_dirty)
	{
		*retVal=(unsigned long)NULL;
		pthread_mutex_unlock(&m_locker);
		return IVT_FAILURE;
	}
	pthread_mutex_unlock(&m_locker);

	alarmDst = &m_alarmData[m_rPos];
	*retVal = (unsigned long)alarmDst;
	m_rPos++;
	if(ELEMENT_SIZE==m_rPos)
		m_rPos = 0;
	sem_post(&m_semW);

	return IVT_SUCCESS;
}

int ivtAlarmRecver::consumerPendOut(int getType)
{
	pthread_mutex_lock(&m_locker);
	m_dirty = 1;
	pthread_mutex_unlock(&m_locker);
	sem_post(&m_semR);
	return IVT_SUCCESS;
}

int ivtAlarmRecver::startIvtAlarmRecver(int alarmPort)
{
	int i;

	m_rPos = m_wPos = 0;
	m_dirty = 0;

	sem_init(&m_semW, 0, ELEMENT_SIZE-1);
	sem_init(&m_semR, 0, 0);

	for(i=0;i<ELEMENT_SIZE;i++)
	{
		m_alarmData[i].params = memalign(16, PARA_BUF_SIZE);

		if(NULL==m_alarmData[i].params)
		{
			IVT_ERR("memalign err!\n");
			stopIvtAlarmRecver();
			return -1;
		}
		memset(m_alarmData[i].params, 0, PARA_BUF_SIZE);
	}

	m_structData.params = memalign(16, PARA_BUF_SIZE);
	if(NULL == m_structData.params)
	{
		IVT_ERR("getType is unknown!\n");
		stopIvtAlarmRecver();
		return -1;
	}
	memset(m_structData.params, 0, PARA_BUF_SIZE);
	m_alarmPort = alarmPort;
	if(create_http_client(HTTP_URL, alarmPort, &m_fd, HTTP_RECV_TIMEOUT)<0)
	{
		stopIvtAlarmRecver();
		return -1;
	}

	return startProducer();
}

int ivtAlarmRecver::stopIvtAlarmRecver()
{
    int i;
	pthread_mutex_lock(&m_killLocker);
	isKill = 1;
	pthread_mutex_unlock(&m_killLocker);
	sem_post(&m_semW);
	shutdown(m_fd,SHUT_RD);

	stopProducer();
	sem_destroy(&m_semW);
	sem_destroy(&m_semR);

	for(i=0;i<ELEMENT_SIZE;i++)
    {
		if(m_alarmData[i].params)
		{
			free(m_alarmData[i].params);
			m_alarmData[i].params = NULL;
		}
	}

	if(m_structData.params)
	{
		free(m_structData.params);
		m_structData.params = NULL;
	}

	close_http_client(m_fd);
	m_fd = -1;

	return IVT_SUCCESS;
}

void ivtAlarmRecver::producerTreadEntry()
{
	ivtRPCStruct *alarmDst;
	timeval timeoutRev;
	fd_set readfds;
	int ret;
	int retVal = 0;
	struct timespec tsNow;
	time_t nextNow;
	ivtRPCAlarmNotify *alarm =(ivtRPCAlarmNotify *)(m_structData.params);

	clock_gettime(CLOCK_MONOTONIC, &tsNow);
	ivt_sendAlarmHeartBeat(m_fd);
	nextNow = tsNow.tv_sec + 120;
	timeoutRev.tv_sec = 120;
	timeoutRev.tv_usec = 0;
	m_structData.rpcType = RPC_REQ;
	m_structData.subType = IVT_ALARMNOTIFY;
	IVT_DEBUG("*********************nextNow %d\n", (unsigned int)nextNow);

	while(1)
	{
	    pthread_mutex_lock(&m_killLocker);
		if(isKill)
		{
			pthread_mutex_unlock(&m_killLocker);
			goto ERR_EXIT;
		}
		pthread_mutex_unlock(&m_killLocker);

		//http connection err, reset http client
		if(-2==retVal)
		{
			close_http_client(m_fd);
			m_fd = -1;
			if(create_http_client(HTTP_URL, m_alarmPort, &m_fd, HTTP_RECV_TIMEOUT))
			{
			    sleep(10);
				continue;
			}
			retVal = 0;
		}

		FD_ZERO(&readfds);
		FD_SET(m_fd,&readfds);

		ret = select(m_fd + 1, &readfds, NULL, NULL, &timeoutRev);
		if(ret > 0)
		{
			retVal = ivt_recvAlarmData(alarm, m_fd);
		    if(!retVal)
		    {
		        if(IVT_ALARM_MD==alarm->type || IVT_ALARM_HEARTBEAT==alarm->type)
		        {
					IVT_DEBUG("**********ALARM MD***********\n");

					sem_wait(&m_semW);
					alarmDst = &m_alarmData[m_wPos];
					alarmDst->rpcType = m_structData.rpcType;
					alarmDst->subType = m_structData.subType;
					//alarmDst->seq = m_structData.seq;
					memcpy(alarmDst->params, m_structData.params, PARA_BUF_SIZE);
					m_wPos++;
					if(ELEMENT_SIZE==m_wPos)
						m_wPos = 0;
					sem_post(&m_semR);
				}
			}
		}

        //send heartBeat
        clock_gettime(CLOCK_MONOTONIC, &tsNow);
		if(tsNow.tv_sec > (nextNow-2) || !timeoutRev.tv_sec)
		{
    		IVT_DEBUG("now %d next %d\n", (unsigned int)tsNow.tv_sec, (unsigned int)nextNow);
			retVal = ivt_sendAlarmHeartBeat(m_fd);
		    nextNow = tsNow.tv_sec + 120;
			timeoutRev.tv_sec = 120;
			timeoutRev.tv_usec = 0;
		}
	}

ERR_EXIT:
	pthread_exit(NULL);
}
























































