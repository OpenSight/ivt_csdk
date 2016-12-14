#include "ivtRPCJson.h"
#include "ivtMacro.h"
#include "ivtStruct.h"
#include "../websocket/websocket.h"
#include "ivtProducerConsumer.h"
#include <semaphore.h>
#include <pthread.h>

#ifndef _IVTRECVSEND_
#define _IVTRECVSEND_

class ivtRPCWebSocketRecv:public ivt_producerConsumerBase
{
	public:
		ivtRPCWebSocketRecv();
		virtual ~ivtRPCWebSocketRecv();
		virtual int consumerGetOneElement(void *element, int getType=0);
		virtual int consumerPendOut(int getType);
		int startIvtRPCWebSocketRecv(wsContext_t *ctx);
		int stopIvtRPCWebSocketRecv();
	private:
		virtual void producerTreadEntry();
    private:
		wsContext_t *m_ctx;
		char m_wsBuf[WS_BUF_SIZE];
		ivtRPCJson m_rpcJson;
		ivtRPCStruct m_reqData[ELEMENT_SIZE];
		ivtRPCStruct m_eventData[ELEMENT_SIZE];
		ivtRPCStruct m_resData[RES_ELEMENT_SIZE];

		ivtRPCStruct m_structData;

		unsigned int m_rEvPos;
		unsigned int m_wEvPos;
		sem_t m_semREv;
		sem_t m_semWEv;

		unsigned int m_rReqPos;
		unsigned int m_wReqPos;
		sem_t m_semRReq;
		sem_t m_semWReq;

		unsigned int m_rResPos;
		unsigned int m_wResPos;
		sem_t m_semRRes;
		sem_t m_semWRes;

		int m_dirtyEv;
		int m_dirtyReq;
		int m_dirtyRes;

		pthread_mutex_t m_locker;
};

//-------------------------------------------------------------------------------
class ivtRPCWebSocketSend:public ivt_producerConsumerBase
{
	public:
		ivtRPCWebSocketSend();
		virtual ~ivtRPCWebSocketSend();

		int setIvtEventCallback(ivtCallBack cb, int eventType); //event send from ivt
		int setIvtReqCallback(ivtCallBack cb, int eventType); //event send from ivt

		virtual int consumerGetOneElement(void *element, int getType=0);
		virtual int consumerPendOut(int getType);
		int startIvtRPCWebSocketSend(wsContext_t *ctx, 
			ivt_producerConsumerBase *producer, ivt_producerConsumerBase *alarmProducer);
		int stopIvtRPCWebSocketSend();

	private:
		virtual void producerTreadEntry();

		int commandChoose();
		int getHourSecTime(int &hour, int &sec);
		//int getSecTime();
    private:
	    ivtCallBack m_ivtEventCb[IVT_ELSE_EVENT+1];
		ivtCallBack m_ivtReqCb[IVT_ELSE_METHOD+1];
		ivt_producerConsumerBase *m_resProducer;
		ivt_producerConsumerBase *m_alarmProducer;
		
		ivtRPCJson m_rpcJson;
		char *m_wsBuf;
		ivtRPCStruct m_command;
		ivtRPCStruct *m_cmdStruct;
		time_t m_fTimeKA;
		time_t m_fTimePV;
		wsContext_t *m_ctx;
		//unsigned int m_secCount;
		int m_firmWare;
		int m_onVideo;
		int m_reqSeq;
		int m_manFirmware;

		char m_outAlarmState[IVT_CHANNEL_NUM];
	    char m_motionState[IVT_CHANNEL_NUM];
};

//-------------------------------------------------------------
class ivtDoIvcReq:public ivt_producerConsumerBase
{
	public:
		ivtDoIvcReq();
		virtual ~ivtDoIvcReq();

		int setIvcReqCallback(ivcCallBack cb, int reqType); //event send from ivt

		virtual int consumerGetOneElement(void *element, int getType=0);
		virtual int consumerPendOut(int getType);
		int startDoIvcReq(wsContext_t *ctx, ivt_producerConsumerBase *producer);
		int stopDoIvcReq();
	private:
		virtual void producerTreadEntry();
	private:

		ivt_producerConsumerBase *m_producer;
		wsContext_t *m_ctx;
		ivtRPCJson m_rpcJson;
		ivcCallBack m_ivcReqCb[IVC_ELSE_METHOD+1];
		char *m_wsBuf;
		ivtRPCStruct m_res;
};

class ivtDoIvcEvent:public ivt_producerConsumerBase
{
	public:
		ivtDoIvcEvent();
		virtual ~ivtDoIvcEvent();
		int setIvcEventCallback(ivtCallBack cb, int eventType); //event send from ivt
		virtual int consumerGetOneElement(void *element, int getType=0);
		virtual int consumerPendOut(int getType);
		int startDoIvcEvent(wsContext_t *ctx, ivt_producerConsumerBase *producer);
		int stopDoIvcEvent();
	private:
		virtual void producerTreadEntry();
	private:
		ivt_producerConsumerBase *m_producer;
		wsContext_t *m_ctx;
		ivtRPCJson m_rpcJson;
		ivtCallBack m_ivcEventCb[IVC_ELSE_EVENT+1];
};

//---------------------------------------------------------------------
class ivtAlarmRecver:public ivt_producerConsumerBase
{
	public: 
		ivtAlarmRecver();
		virtual ~ivtAlarmRecver();
		virtual int consumerGetOneElement(void *element, int getType=0);
		virtual int consumerPendOut(int getType);
		int startIvtAlarmRecver(int alarmPort);
		int stopIvtAlarmRecver();
	private:
		virtual void producerTreadEntry();
	private:
		ivt_producerConsumerBase *m_producer;//to get res, and we should a filiter in recver
		int m_alarmPort;

		ivtRPCStruct m_alarmData[ELEMENT_SIZE];
		ivtRPCStruct m_structData;

		unsigned int m_rPos;
		unsigned int m_wPos;
        int m_fd;
		int m_dirty;
		sem_t m_semR;
		sem_t m_semW;
		pthread_mutex_t m_locker;
};

#endif



















































