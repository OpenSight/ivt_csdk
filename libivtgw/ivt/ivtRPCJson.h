#include <string.h>
#include <stdio.h>
#include "ivtStruct.h"
#include "ivtMacro.h"
#include "../libJson/json.h"
#include <string>

#ifndef _IVTRPCJSON_
#define _IVTRPCJSON_

using namespace std;
class ivtRPCJson
{
	public:
		ivtRPCJson();
		virtual ~ivtRPCJson();
		int ivcPayLoadToStructData(char *pJsonData,  ivtRPCStruct *pStructData);
		int ivtStructDataToPayLoad(char *pJsonData,  ivtRPCStruct *pStructData, int jsonDataLen);
	private:
		int readIVCReqParams(ivtRPCStruct *pStructData, Json::Value *paraObj);//req from ivc
		int readIVCResParams(ivtRPCStruct *pStructData, Json::Value *paraObj);//res from ivc
		int readIVCEventParams(ivtRPCStruct *pStructData, Json::Value *paraObj);//event from ivc
        int readIVCReqParamsStartCR(ivtRPCStartCR *cr, Json::Value *paraObj);
		int readIVCReqParamsAlarmMDC(ivtRPCMDC *mdc, Json::Value *paraObj);		
		int readIVCReqParamsAlarmRectDC(ivtRPCRectDC *rectDC, Json::Value *paraObj);
		
		int writeIVTReqParams(ivtRPCStruct *pStructData, Json::Value *paraObj);//ivt req send
		int writeIVTResParams(ivtRPCStruct *pStructData, Json::Value *paraObj);//ivt res send
		int writeIVTEventParams(ivtRPCStruct *pStructData, Json::Value *paraObj);//ivt event send
        
	private:

		//static const char m_payLoadType[][];
		static const char *m_RPCType[ ];
		static const char *m_ivt_methodType[ ];
		static const char *m_ivc_methodType[ ];
		static const char *m_ivt_eventType[ ];
		static const char *m_ivc_eventType[ ];
        static const string m_ivt_videoDType[ ];
		static const string m_ivt_ptzType[ ];
		static const string m_IVT_PTZ_SSType[];
		static const string m_IVT_alarm_SSType[];
};

#endif





















































