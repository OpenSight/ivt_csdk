#include <pthread.h>

#ifndef _IVTPRODUCERCONSUMER_
#define _IVTPRODUCERCONSUMER_

//construction function  deconstruction funtion
class ivt_producerConsumerBase
{
	public:
		ivt_producerConsumerBase();
		virtual ~ivt_producerConsumerBase();
		virtual int consumerGetOneElement(void *element, int getType=0)=0;
	    virtual int consumerPendOut(int getType)=0;
	protected:
		virtual void producerTreadEntry(void)=0;
		int startProducer();
		int stopProducer();

	private:
		static void * _Entry(void *inTread);
	protected:
		pthread_t PThreadID;
		int isStart;
		int isKill;
		pthread_mutex_t m_killLocker;
};


#endif


