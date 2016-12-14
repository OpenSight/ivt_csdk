#include "ivtMacro.h"
#include <pthread.h>
#include <semaphore.h>
#include "ivtProducerConsumer.h"
#include "ivtMacro.h"

ivt_producerConsumerBase::ivt_producerConsumerBase()
{
	isStart = 0;
	pthread_mutex_init(&m_killLocker, NULL);
}

ivt_producerConsumerBase::~ivt_producerConsumerBase()
{
	pthread_mutex_destroy(&m_killLocker);
}

void* ivt_producerConsumerBase::_Entry(void *inThread)//static
{
	ivt_producerConsumerBase* theThread = (ivt_producerConsumerBase*)inThread;
    theThread->producerTreadEntry();
    return 0;
}

int ivt_producerConsumerBase::startProducer()
{
	if(isStart)
	{
		IVT_ERR("AIPU_OP_ORDER_ERR:arealy start!\n");
		return IVT_OP_ORDER_ERR;
	}
    isKill = 0;
    if(pthread_create((pthread_t*)&PThreadID, NULL, _Entry, (void*)this)<0)//this->producerTreadEntry
    {
    	IVT_ERR("pthread_create error!\n");
		return IVT_INNER_ERR;
    }
	isStart = 1;
	return IVT_SUCCESS;
}



int ivt_producerConsumerBase::stopProducer()
{
  if(isStart)
  {
  	pthread_join((pthread_t)PThreadID, NULL);
    isStart = 0;
  }
  return IVT_SUCCESS;
}








