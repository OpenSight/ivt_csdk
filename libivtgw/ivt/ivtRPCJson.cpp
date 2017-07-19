#include <string.h>
#include "ivtRPCJson.h"
#include "ivtMacro.h"
#include "ivtStruct.h"

ivtRPCJson::ivtRPCJson()
{

}

ivtRPCJson::~ivtRPCJson()
{

}

const char * ivtRPCJson::m_RPCType[ ] = {"req", "event", "resp", "err"};
const char * ivtRPCJson::m_ivt_methodType[ ] = {"preview_server", "GetFirmware", "Keepalive", "AlarmNotify"};
const char * ivtRPCJson::m_ivc_methodType[ ] = {"RTMPPublish", "RTMPStopPublish", "RebootChannel",
	                                            "GetPTZPresetList", "GetPTZPresetTourList",
	                                            "StartCloudRecord", "StopCloudRecord",
	                                            "AlarmMoveDetectConfig", "AlarmRectIntrusionDetectConfig",
												"GetNetConfig"};

const char * ivtRPCJson::m_ivt_eventType[ ]  = {"UNKNOWN_EVENT"};
const char * ivtRPCJson::m_ivc_eventType[ ] = {"CtrlPTZ", "GotoPTZPreset", "CtrlPTZPresetTour",
	                                              "CtrlPTZPatrol", "SyncTime", 
	                                                   "UpgradeFirmware", "UNKNOWN_EVENT"};

const string ivtRPCJson::m_ivt_videoDType[ ] = {"ld", "sd", "hd", "fhd"};
const string ivtRPCJson::m_ivt_ptzType[ ] = {"stop", "upleft","up", "upright",
	                                           "left", "right", "downleft",
	                                             "down","downright", "zoomout", "zoomin"};

const string ivtRPCJson::m_IVT_PTZ_SSType[ ] = {"start","stop"};
const string ivtRPCJson::m_IVT_alarm_SSType[ ] = {"start","end"};


int ivtRPCJson::readIVCEventParams(ivtRPCStruct *pStructData, Json::Value *paraObj)
{
    bool bRet = false;
	int i,temp;
	string strTemp;

	switch(pStructData->subType)
	{
		case IVC_CTRLPTZ:
		{
			ivtRPCPtzCtrl *ptz;
			ptz = (ivtRPCPtzCtrl *)(pStructData->params);

			bRet = paraObj->isMember("channel");
			if(false==bRet)
			{
				IVT_ERR("channel is not exist.\n");
				return -1;
			}
			ptz->channel= (*paraObj)["channel"].asInt();

			bRet = paraObj->isMember("value");
			if(false==bRet)
			{
				IVT_ERR("value is not exist.\n");
				return -1;
			}
			ptz->speed = (*paraObj)["value"].asInt();
			/*
			if(ptz->speed<0 || ptz->speed>100)
				return -1;
			*/

			bRet = paraObj->isMember("op");
			if(false==bRet)
			{
				IVT_ERR("quality is not exist.\n");
				return -1;
			}

			strTemp = (*paraObj)["op"].asString();
			ptz->opType = IVT_PTZ_UNKNOWN;
			for(i=0;i<IVT_PTZ_UNKNOWN;i++)
			{
				if(strTemp==m_ivt_ptzType[i])
				{
					ptz->opType = i;
					break;
				}
			}
		}
		break;
	    case IVC_GOTOPTZPRESET:
		{
			ivtRPCGotoPTZPreset *ptz;
			ptz = (ivtRPCGotoPTZPreset *)(pStructData->params);
			bRet = paraObj->isMember("channel");
			if(false==bRet)
			{
				IVT_ERR("channel is not exist.\n");
				return -1;
			}
			ptz->channel= (*paraObj)["channel"].asInt();

			bRet = paraObj->isMember("token");
			if(false==bRet)
			{
				IVT_ERR("quality is not exist.\n");
				return -1;
			}
			strTemp = (*paraObj)["token"].asString();

			temp = sizeof(ptz->token);
			strncpy(ptz->token, strTemp.c_str(), temp);
			ptz->token[temp-1] = 0;
	    }
		break;
	    case IVC_CTRLPTZPRESETTOUR:
		{
			ivtRPCCtrlPTZPresetTour *ptz;
			ptz = (ivtRPCCtrlPTZPresetTour *)(pStructData->params);
			bRet = paraObj->isMember("channel");
			if(false==bRet)
			{
				IVT_ERR("channel is not exist.\n");
				return -1;
			}
			ptz->channel= (*paraObj)["channel"].asInt();

			bRet = paraObj->isMember("token");
			if(false==bRet)
			{
				ptz->token[0] = 0; //memset
			}
			else
			{
				strTemp = (*paraObj)["token"].asString();
				temp = sizeof(ptz->token);
				strncpy(ptz->token, strTemp.c_str(), temp);
				ptz->token[temp-1] = 0;
			}

			bRet = paraObj->isMember("op");
			if(false==bRet)
			{
				IVT_ERR("quality is not exist.\n");
				return -1;
			}
			strTemp = (*paraObj)["op"].asString();
			ptz->opType = IVT_PTZ_SS_UNKNOWN;
			for(i=0;i<IVT_PTZ_SS_UNKNOWN;i++)
			{
				if(strTemp==m_IVT_PTZ_SSType[i])
				{
					ptz->opType = i;
					break;
				}
			}
	    }
		break;
	    case IVC_CTRLPTZPATROL:
		{
			ivtRPCCtrlPTZPatrol *ptz;
			ptz = (ivtRPCCtrlPTZPatrol *)(pStructData->params);
			bRet = paraObj->isMember("channel");
			if(false==bRet)
			{
				IVT_ERR("channel is not exist.\n");
				return -1;
			}
			ptz->channel= (*paraObj)["channel"].asInt();
			bRet = paraObj->isMember("op");
			if(false==bRet)
			{
				IVT_ERR("quality is not exist.\n");
				return -1;
			}

			strTemp = (*paraObj)["op"].asString();
			ptz->opType = IVT_PTZ_SS_UNKNOWN;
			for(i=0;i<IVT_PTZ_SS_UNKNOWN;i++)
			{
				if(strTemp==m_IVT_PTZ_SSType[i])
				{
					ptz->opType = i;
					break;
				}
			}
	    }
		break;
		case IVC_UPGRADEFIRMWARE:
			break;
		case IVC_SYNCTIME:
			ivtRPCSyncTime *synTime;
			tm tmp_time;
			synTime = (ivtRPCSyncTime *)(pStructData->params); 
			
			bRet = paraObj->isMember("datetime");
			if(false==bRet)
			{
				IVT_ERR("datetime is not exist.\n");
				return -1;
			}			
			strTemp = (*paraObj)["datetime"].asString();
			sscanf(strTemp.c_str(), "%d-%d-%dT%d:%d:%d", &tmp_time.tm_year,
				&tmp_time.tm_mon, &tmp_time.tm_mday, &tmp_time.tm_hour,
				&tmp_time.tm_min, &tmp_time.tm_sec);
			tmp_time.tm_isdst = 0;

			bRet = paraObj->isMember("offset");
			if(false==bRet)
			{
				IVT_ERR("offset is not exist.\n");
				return -1;
			}
			synTime->offsetSec = (*paraObj)["offset"].asInt();
			synTime->year = tmp_time.tm_year;
			synTime->mon = tmp_time.tm_mon;
		    synTime->day = tmp_time.tm_mday;
		    synTime->hour = tmp_time.tm_hour;
		    synTime->minute = tmp_time.tm_min;
		    synTime->sec = tmp_time.tm_sec;
			tmp_time.tm_year-=1900;
			synTime->dateTimeInSec = mktime(&tmp_time);
			//IVT_DEBUG("0 dateTimeInSec %d.\n", synTime->dateTimeInSec);			
			break;
		default:
			return -1;
		    break;
	}

	return 0;
}

int ivtRPCJson::readIVCResParams(ivtRPCStruct *pStructData, Json::Value *paraObj)
{
	bool bRet = false;
	int temp;
	string strTemp;

	switch(pStructData->subType)
	{
	   case IVT_KEEPALIVE:
	   break;
	   case IVT_PREVIEW_SERVER:
	   {
		   ivcRPCPreviewRes *snapRes;
		   snapRes = (ivcRPCPreviewRes *)(pStructData->params);
		   bRet = paraObj->isMember("url");
		   if(false==bRet)
		   {
			   IVT_ERR("url is not exist.\n");
			   return -1;
		   }
		   strTemp = (*paraObj)["url"].asString();
		   temp = sizeof(snapRes->url);
		   strncpy(snapRes->url, strTemp.c_str(), temp);
		   snapRes->url[temp-1] = 0;
	   }
	   break;
	   case IVT_GET_FIRMWARE:
	   {
		   ivcRPCFirmwareRes *firmware;
		   firmware = (ivcRPCFirmwareRes *)(pStructData->params);
		   //bRet =(*paraObj)["version"].empty();
		   firmware->forceToUpd = 0;
		   bRet = paraObj->isMember("firmware_model");
		   if(false==bRet)
		   {
			   IVT_ERR("firmware_model is not exist.\n");
			   return -1;
		   }
		   strTemp = (*paraObj)["firmware_model"].asString();
		   temp = sizeof(firmware->version);
		   strncpy(firmware->version, strTemp.c_str(), temp);
		   firmware->version[temp-1] = 0;
		   //bRet = paraObj["url"]->empty();
		   bRet = paraObj->isMember("url");
		   if(false==bRet)
		   {
			   IVT_ERR("url is not exist.\n");
			   return -1;
		   }
		   strTemp = (*paraObj)["url"].asString();
		   temp = sizeof(firmware->url);
		   strncpy(firmware->url, strTemp.c_str(), temp);
		   firmware->url[temp-1] = 0;
	   }
	   break;
	   case IVT_ALARMNOTIFY:
	   {
	   	   ivcRPCAlarmRes *alarmRes;
		   alarmRes = (ivcRPCAlarmRes *)(pStructData->params);
		   bRet = paraObj->isMember("pic_upload_url");
		   if(false==bRet)
		   {
			   IVT_ERR("url is not exist.\n");
			   return -1;
		   }
		   strTemp = (*paraObj)["pic_upload_url"].asString();
		   temp = sizeof(alarmRes->url);
		   strncpy(alarmRes->url, strTemp.c_str(), temp);
		   alarmRes->url[temp-1] = 0;
	   }
	   break;
	   default:
		   return -1;
	   break;
	}

	return 0;
}

int ivtRPCJson::readIVCReqParamsStartCR(ivtRPCStartCR *cr, Json::Value *paraObj)
{
    bool bRet = false;
	int i,temp;
	string strTemp;

	bRet = paraObj->isMember("session_id");
	if(false==bRet)
	{
		IVT_ERR("session_id is not exist.\n");
		return -1;
	}
	strTemp = (*paraObj)["session_id"].asString();
	temp = sizeof(cr->recSession);
	strncpy(cr->recSession, strTemp.c_str(), sizeof(cr->recSession));
	cr->recSession[temp-1] = 0;

	bRet = paraObj->isMember("channel");
	if(false==bRet)
	{
		IVT_ERR("channel is not exist.\n");
		return -1;
	}
	cr->channel= (*paraObj)["channel"].asInt();

	bRet = paraObj->isMember("quality");
	if(false==bRet)
	{
		IVT_ERR("quality is not exist.\n");
		return -1;
	}
	strTemp = (*paraObj)["quality"].asString();
	cr->qaulity = IVT_XHD;
	for(i=0;i<IVT_XHD;i++)
	{
		if(strTemp==m_ivt_videoDType[i])
		{
			cr->qaulity = i;
			break;
		}
	}

	bRet = paraObj->isMember("seg_duration");
	if(false==bRet)
	{
		IVT_ERR("seg_duration is not exist.\n");
		return -1;
	}
	cr->seg_duration = (*paraObj)["seg_duration"].asInt();

	bRet = paraObj->isMember("seg_max_size");
	if(false==bRet)
	{
		IVT_ERR("seg_max_size is not exist.\n");
		return -1;
	}
	cr->seg_max_size = (*paraObj)["seg_max_size"].asInt();

	bRet = paraObj->isMember("seg_max_count");
	if(false==bRet)
	{
		IVT_ERR("seg_max_count is not exist.\n");
		return -1;
	}
	cr->seg_max_count = (*paraObj)["seg_max_count"].asInt();

	bRet = paraObj->isMember("prerecord_seconds");
	if(false==bRet)
	{
		IVT_ERR("prerecord_seconds is not exist.\n");
		return -1;
	}
	cr->prerecord_seconds = (*paraObj)["prerecord_seconds"].asInt();

	bRet = paraObj->isMember("start_ts");
	if(false==bRet)
	{
		IVT_ERR("start_ts is not exist.\n");
		return -1;
	}
	cr->start_ts = (*paraObj)["start_ts"].asInt();

	if(cr->start_ts<0)
		cr->start_ts = 0;
	
	bRet = paraObj->isMember("cbk_url");
	if(false==bRet)
	{
		IVT_ERR("url is not exist.\n");
		return -1;
	}
	strTemp = (*paraObj)["cbk_url"].asString();
	temp = sizeof(cr->url);
	strncpy(cr->url, strTemp.c_str(), sizeof(cr->url));
	cr->url[temp-1] = 0;
    //-------------------------------------------------
	bRet = paraObj->isMember("max_bitrate");
	if(false==bRet)
	{
		cr->max_bitrate = 0;
		return 0;
	}
	
	temp = (*paraObj)["max_bitrate"].asInt();
	cr->max_bitrate = temp>>10;
	
	if(cr->max_bitrate<0)
		cr->max_bitrate = 0;
    
	return 0;
}

int ivtRPCJson::readIVCReqParamsAlarmMDC(ivtRPCMDC *mdc, Json::Value *paraObj)
{
    bool bRet = false;
	bool enc;
	//int i,temp;

	bRet = paraObj->isMember("channel");
	if(false==bRet)
	{
		IVT_ERR("channel is not exist.\n");
		return -1;
	}
	mdc->channel= (*paraObj)["channel"].asInt();

	bRet = paraObj->isMember("enable");
	if(false==bRet)
	{
		IVT_ERR("enable is not exist.\n");
		return -1;
	}
	enc = (*paraObj)["enable"].asBool();
    mdc->enable = (int)enc;
	
	bRet = paraObj->isMember("start");
	if(false==bRet)
	{
		mdc->start = 0;
	}
	else
		mdc->start= (*paraObj)["start"].asInt();

	bRet = paraObj->isMember("end");
	if(false==bRet)
	{
		mdc->end = 86400;
	}
	else
		mdc->end= (*paraObj)["end"].asInt();

	if(mdc->end>86400)
		mdc->end = 86400;

	mdc->start1 = -1;
	if(mdc->end < mdc->start)
	{
		mdc->end1 = mdc->end;
		mdc->end = 86400;	
		mdc->start1 = 0; 
		//IVT_ERR("end <= start.\n");
		//return -1;
	}
	
	bRet = paraObj->isMember("sensitivity");
	if(false==bRet)
	{
		IVT_ERR("sensitivity is not exist.\n");
		return -1;
	}
	mdc->sensitivity= (*paraObj)["sensitivity"].asInt();

	bRet = paraObj->isMember("delay");
	if(false==bRet)
	{
		IVT_ERR("delay is not exist.\n");
		return -1;
	}
	mdc->delay= (*paraObj)["delay"].asInt();
	
	return 0;
}

int ivtRPCJson::readIVCReqParamsAlarmRectDC(ivtRPCRectDC *rectDC, Json::Value *paraObj)
{
    bool bRet = false;
	bool enc;
	Json::Value rectObj;
		
	//int i,temp;

	bRet = paraObj->isMember("channel");
	if(false==bRet)
	{
		IVT_ERR("channel is not exist.\n");
		return -1;
	}
	rectDC->channel= (*paraObj)["channel"].asInt();

	bRet = paraObj->isMember("enable");
	if(false==bRet)
	{
		IVT_ERR("enable is not exist.\n");
		return -1;
	}
	enc = (*paraObj)["enable"].asBool();
    rectDC->enable = (int)enc;
	
	bRet = paraObj->isMember("start");
	if(false==bRet)
	{
		rectDC->start = 0;
	}
	else
		rectDC->start= (*paraObj)["start"].asInt();

	bRet = paraObj->isMember("end");
	if(false==bRet)
	{
		rectDC->end = 86400;
	}
	else
		rectDC->end= (*paraObj)["end"].asInt();

	if(rectDC->end>86400)
		rectDC->end = 86400;

	rectDC->start1 = -1;
	if(rectDC->end < rectDC->start)
	{
		rectDC->end1 = rectDC->end;
		rectDC->end = 86400;	
		rectDC->start1 = 0; 
		//IVT_ERR("end <= start.\n");
		//return -1;
	}
	
	bRet = paraObj->isMember("sensitivity");
	if(false==bRet)
	{
		IVT_ERR("sensitivity is not exist.\n");
		return -1;
	}
	rectDC->sensitivity= (*paraObj)["sensitivity"].asInt();

	bRet = paraObj->isMember("delay");
	if(false==bRet)
	{
		IVT_ERR("delay is not exist.\n");
		return -1;
	}
	rectDC->delay= (*paraObj)["delay"].asInt();

    bRet = paraObj->isMember("rect");
	if(true==bRet)
	{
		rectObj = (*paraObj)["rect"];
		bRet = rectObj.isMember("ulx");
		if(false==bRet)
		{
			IVT_ERR("ulx is not exist.\n");
			return -1;
		}
		rectDC->ulx= rectObj["ulx"].asInt();	

		bRet = rectObj.isMember("uly");
		if(false==bRet)
		{
			IVT_ERR("uly is not exist.\n");
			return -1;
		}
		rectDC->uly= rectObj["uly"].asInt();	

		bRet = rectObj.isMember("urx");
		if(false==bRet)
		{
			IVT_ERR("urx is not exist.\n");
			return -1;
		}
		rectDC->urx= rectObj["urx"].asInt();

		bRet = rectObj.isMember("ury");
		if(false==bRet)
		{
			IVT_ERR("ury is not exist.\n");
			return -1;
		}
		rectDC->ury= rectObj["ury"].asInt();	

        bRet = rectObj.isMember("dlx");
		if(false==bRet)
		{
			IVT_ERR("dlx is not exist.\n");
			return -1;
		}
		rectDC->dlx= rectObj["dlx"].asInt();	

		bRet = rectObj.isMember("dly");
		if(false==bRet)
		{
			IVT_ERR("dly is not exist.\n");
			return -1;
		}
		rectDC->dly= rectObj["dly"].asInt();	

		bRet = rectObj.isMember("drx");
		if(false==bRet)
		{
			IVT_ERR("drx is not exist.\n");
			return -1;
		}
		rectDC->drx= rectObj["drx"].asInt();	

		bRet = rectObj.isMember("dry");
		if(false==bRet)
		{
			IVT_ERR("dry is not exist.\n");
			return -1;
		}
		rectDC->dry= rectObj["dry"].asInt();		
	}
	
	return 0;
}

int ivtRPCJson::readIVCReqParams(ivtRPCStruct *pStructData, Json::Value *paraObj)
{
    bool bRet = false;
	int i,temp;
	string strTemp;

	switch(pStructData->subType)
	{
		case IVC_RTMPPUBLISH:
		{
			ivtRPCRtmpPublish *rtmp;
			rtmp = (ivtRPCRtmpPublish *)(pStructData->params);
		   // bRet = paraObj["url"]->empty();
		    bRet = paraObj->isMember("url");
			if(false==bRet)
			{
				IVT_ERR("IVC_RTMPPUBLISH url is not exist.\n");
				return -1;
			}
			strTemp = (*paraObj)["url"].asString();
			temp = sizeof(rtmp->url);
            strncpy(rtmp->url, strTemp.c_str(), sizeof(rtmp->url));
            rtmp->url[temp-1] = 0;
			//bRet = paraObj["stream_Id"]->empty();
            bRet = paraObj->isMember("stream_id");
			if(false==bRet)
			{
				IVT_ERR("IVC_RTMPPUBLISH stream_id is not exist.\n");
				return -1;
			}
			strTemp = (*paraObj)["stream_id"].asString();
			temp = sizeof(rtmp->streamID);
            strncpy(rtmp->streamID, strTemp.c_str(), sizeof(rtmp->streamID));
			rtmp->streamID[temp-1] = 0;
			//bRet = paraObj["quality"]->empty();
			 bRet = paraObj->isMember("quality");
			if(false==bRet)
			{
				IVT_ERR("IVC_RTMPPUBLISH quality is not exist.\n");
				return -1;
			}

			strTemp = (*paraObj)["quality"].asString();
			rtmp->qaulity = IVT_XHD;
            for(i=0;i<IVT_XHD;i++)
            {
				if(strTemp==m_ivt_videoDType[i])
				{
					rtmp->qaulity = i;
					break;
				}
            }

			//bRet = paraObj["channel"]->empty();
            bRet = paraObj->isMember("channel");
			if(false==bRet)
			{
				IVT_ERR("IVC_RTMPPUBLISH channel is not exist.\n");
				return -1;
			}
			rtmp->channel= (*paraObj)["channel"].asInt();

			bRet = paraObj->isMember("max_bitrate");
			if(false==bRet)
			{
				//IVT_ERR("IVC_RTMPPUBLISH max_bitrate is not exist.\n");
				rtmp->max_bitrate = 0;
				return 0;
			}
			temp = (*paraObj)["max_bitrate"].asInt();
			rtmp->max_bitrate = temp>>10;
			if(rtmp->max_bitrate<0)
				rtmp->max_bitrate = 0;
			
		}
		break;
		case IVC_RTMPSTOPPUBLISH:
		{
			ivtRPCStopPublish *stop;
			stop = (ivtRPCStopPublish *)(pStructData->params);
			//bRet = paraObj["stream_Id"]->empty();
			bRet = paraObj->isMember("stream_id");
			if(false==bRet)
			{
				IVT_ERR("IVC_RTMPSTOPPUBLISH stream_id is not exist.\n");
				return -1;
			}
			strTemp = (*paraObj)["stream_id"].asString();
			temp = sizeof(stop->streamID);
			strncpy(stop->streamID, strTemp.c_str(), sizeof(stop->streamID));
			stop->streamID[temp-1] = 0;
			//bRet = paraObj["channel"]->empty();
			bRet = paraObj->isMember("channel");
			if(false==bRet)
			{
				IVT_ERR("IVC_RTMPPUBLISH channel is not exist.\n");
				return -1;
			}
			stop->channel= (*paraObj)["channel"].asInt();
		}
		break;
		case IVC_REBOOTCHANNEL:
		{
			ivtRPCRebootChl *pBoot;
			pBoot = (ivtRPCRebootChl *)(pStructData->params);
			bRet = paraObj->isMember("channel");
			if(false==bRet)
			{
				IVT_ERR("IVC_RTMPPUBLISH channel is not exist.\n");
				return -1;
			}
			pBoot->channel= (*paraObj)["channel"].asInt();
		}
		break;
		case IVC_GETPTZPRESETLIST:
		{
			ivtRPCPTZPreList *ptz;
			ptz = (ivtRPCPTZPreList *)(pStructData->params);
			bRet = paraObj->isMember("channel");
			if(false==bRet)
			{
				IVT_ERR("IVC_RTMPPUBLISH channel is not exist.\n");
				return -1;
			}
			ptz->channel= (*paraObj)["channel"].asInt();
		}
		break;
		case IVC_GETPTZPRESETTOURLIST:
		{
			ivtRPCPTZPreTList *ptz;
			ptz = (ivtRPCPTZPreTList *)(pStructData->params);
			bRet = paraObj->isMember("channel");
			if(false==bRet)
			{
				IVT_ERR("IVC_RTMPPUBLISH channel is not exist.\n");
				return -1;
			}
			ptz->channel= (*paraObj)["channel"].asInt();
		}
		break;
		case IVC_STARTCLOUDRECORD:
		{
			ivtRPCStartCR *cr;
			cr = (ivtRPCStartCR *)(pStructData->params);
			if(readIVCReqParamsStartCR(cr, paraObj)<0)
			{
				return -1;
			}
		}
		break;
		case IVC_ALARMMOVEDETECTCONFIG:
		{
			ivtRPCMDC *mdc;
			mdc = (ivtRPCMDC *)(pStructData->params);
			if(readIVCReqParamsAlarmMDC(mdc, paraObj)<0)
			{
				return -1;
			}
		}
		break;
		case IVC_ALARMRECTDETECTCONFIG:
		{
            ivtRPCRectDC *rectDC;
			rectDC = (ivtRPCRectDC *)(pStructData->params);
			if(readIVCReqParamsAlarmRectDC(rectDC, paraObj)<0)
			{
				return -1;
			}
		}
		case IVC_STOPCLOUDRECORD:
		{
			ivtRPCStopCR *cr;
			cr = (ivtRPCStopCR *)(pStructData->params);

			bRet = paraObj->isMember("session_id");
			if(false==bRet)
			{
				IVT_ERR("session_id is not exist.\n");
				return -1;
			}
			strTemp = (*paraObj)["session_id"].asString();
			temp = sizeof(cr->recSession);
			strncpy(cr->recSession, strTemp.c_str(), sizeof(cr->recSession));
			cr->recSession[temp-1] = 0;
			
			bRet = paraObj->isMember("channel");
			if(false==bRet)
			{
				IVT_ERR("channel is not exist.\n");
				return -1;
			}
			cr->channel= (*paraObj)["channel"].asInt();
		}
		break;
		case IVC_GETNETCONFIG:
		{
			ivtRPCGeneral *generalParams;
			generalParams = (ivtRPCGeneral *)(pStructData->params);
			bRet = paraObj->isMember("channel");
			if(false==bRet)
			{
				IVT_ERR("channel is not exist.\n");
				return -1;
			}
			generalParams->channel = (*paraObj)["channel"].asInt();
		}
		break;
		default:
			return -1;
		break;
	}

	return 0;
}

int ivtRPCJson::ivcPayLoadToStructData(char *pJsonData,  ivtRPCStruct *pStructData)
{
	bool bRet = false;
	int i, temp;
	string strTemp;
	Json::Reader reader;
	Json::Value value;
	Json::Value item;

	if(!pStructData->params)
		return -1;

	bRet = reader.parse(pJsonData, value);
	if(false==bRet)
	{
	    IVT_ERR("ivcPayLoadToStructData reader parse error.\n");
	    return -1;
	}

    pStructData->rpcType = RPC_ELSE;
	for(i=0;i<RPC_ELSE;i++)
	{
		//bRet = value[m_RPCType[i]].empty();
		bRet = value.isMember(m_RPCType[i]);
		if(true==bRet)
		{
		    pStructData->rpcType = i;
			break;
		}
	}

	switch(pStructData->rpcType)
	{
	    case RPC_REQ:
		{
			strTemp = value[m_RPCType[RPC_REQ]].asString();
			pStructData->subType = IVC_ELSE_METHOD;
			for(i=0;i<IVC_ELSE_METHOD;i++)
			{
				if(!strcmp(m_ivc_methodType[i], strTemp.c_str()))
				{
					pStructData->subType = i;
					break;
				}
			}

			//bRet = value["seq"].empty();
			bRet = value.isMember("seq");
			if(false==bRet)
			{
				IVT_ERR("ivcPayLoadToStructData seq is not exist.\n");
				return -1;
			}
			pStructData->seq = value["seq"].asInt();

			//bRet = value["params"].empty();
			bRet = value.isMember("params");
			if(true==bRet)
			{
				item = value["params"];
				if(readIVCReqParams(pStructData, &item)<0)
				{
					pStructData->subType = IVC_ELSE_METHOD;
					return 0;//we send err response to sender
				}
			}
	    }
		break;
	    case RPC_EVENT:
		{
			strTemp = value[m_RPCType[RPC_EVENT]].asString();
			pStructData->subType = IVC_ELSE_EVENT;
			for(i=0;i<IVC_ELSE_EVENT;i++)
			{
				if(!strcmp(m_ivc_eventType[i], strTemp.c_str()))
				{
					pStructData->subType = i;
					break;
				}
			}

			//bRet = value["params"].empty();
			bRet = value.isMember("params");
			if(true==bRet)
			{
				item = value["params"];
				if(readIVCEventParams(pStructData, &item)<0)
					return -1;
			}
	    }
		break;
	    case RPC_RESP:
		{
			bRet = value.isMember("seq");
			if(false==bRet)
			{
				IVT_ERR("ivcPayLoadToStructData seq is not exist.\n");
				return -1;
			}
			pStructData->seq = value["seq"].asInt();
			item = value["resp"];
			if(readIVCResParams(pStructData, &item)<0)
				return -1;
	    }
		break;
		case RPC_ERR:  //ok
	    {
			ivtRPCErr *err;
			//bRet = value["seq"].empty();
			bRet = value.isMember("seq");
			if(false==bRet)
			{
				IVT_ERR("ivcPayLoadToStructData seq is not exist.\n");
				return -1;
			}
			pStructData->seq = value["seq"].asInt();

            item = value[m_RPCType[RPC_ERR]];
            //bRet = item["code"].empty();
            bRet = item.isMember("code");
			if(false==bRet)
			{
				IVT_ERR("ivcPayLoadToStructData seq is not exist.\n");
				return -1;
			}
            err = (ivtRPCErr *)(pStructData->params);
            err->errCode = item["code"].asInt();
			strTemp = item["msg"].asString();
			temp = sizeof(err->msg);
            strncpy(err->msg, strTemp.c_str(), temp);
			err->msg[temp-1] = 0;
		}
		break;
		default:
			IVT_ERR("ivcPayLoadToStructData RPC_ELSE.\n");
			return -1;
		break;
	}
	return 0;
}

int ivtRPCJson::writeIVTReqParams(ivtRPCStruct *pStructData, Json::Value *paraObj)
{
    Json::Value arrayObj;
	Json::Value item;
	int i;

	switch(pStructData->subType)
	{
		case IVT_KEEPALIVE:
		{
			ivtRPCKeepAlive *liveEvent;
			liveEvent = (ivtRPCKeepAlive *)(pStructData->params);
			(*paraObj)["state"] = liveEvent->state;

			if(liveEvent->chnlNum)
			{
				for(i=0;i<liveEvent->chnlNum;i++)
				{
					int temp0, temp1, temp2;
				    item.clear();
					item["state"] = liveEvent->chnlState[i];
					item["channel"] = liveEvent->chnlID[i];
					item["record_session"] = liveEvent->recSession[i];
                                  				   
				    temp0 = '1'==liveEvent->motionState[i] ? 4:0;//100
					temp1 = '1'==liveEvent->outAlarmState[i] ? 2:0;//10
					temp2 = '1'==liveEvent->rectState[i] ? 8:0;//10
					
					item["alarm"] = temp0|temp1|temp2;
					
					arrayObj.append(item);					
				}

				(*paraObj)["channels"] = arrayObj;
			}
			else
			{
				(*paraObj)["channels"].resize(0);
			}
		}
		break;
		case IVT_PREVIEW_SERVER:
			return -1;
		break;
		case IVT_GET_FIRMWARE:
			return -1;	
		break;
		case IVT_ALARMNOTIFY:
		{
			ivtRPCAlarmNotify *alarmReq;
			alarmReq = (ivtRPCAlarmNotify *)(pStructData->params);
			
			(*paraObj)["channel"] = alarmReq->channel;
			(*paraObj)["type"] = alarmReq->type;
	        (*paraObj)["state"] = m_IVT_alarm_SSType[alarmReq->state];
			(*paraObj)["desc"] = alarmReq->desc;
		}
		break;
		default:
			IVT_ERR("writeIVTReqParams RPC_ELSE.\n");
			return -1;
		break;
	}

	return 0;
}

int ivtRPCJson::writeIVTResParams(ivtRPCStruct *pStructData, Json::Value *paraObj)
{
	Json::Value item;
    int i;

	switch(pStructData->subType)
	{
		case IVC_RTMPPUBLISH:
		case IVC_RTMPSTOPPUBLISH:
		case IVC_REBOOTCHANNEL:
		case IVC_STARTCLOUDRECORD:
		case IVC_STOPCLOUDRECORD:
		case IVC_ALARMMOVEDETECTCONFIG:
			return -1;
		break;
	    case IVC_GETPTZPRESETLIST:
		{
			ivtRPCPTZPreListR *ptz = (ivtRPCPTZPreListR *)(pStructData->params);
			if(ptz->tNum)
			{
				for(i=0;i<ptz->tNum;i++)
	            {
					item["name"] = ptz->name[i];
					item["token"] = ptz->token[i];
					(*paraObj).append(item);
				}
			}
			else
			{
				(*paraObj).resize(0);
			}
    	}
		break;
		case IVC_GETPTZPRESETTOURLIST:
		{
			ivtRPCPTZPreTListR *ptz = (ivtRPCPTZPreTListR *)(pStructData->params);
			if(!ptz->tNum)
			{
				for(i=0;i<ptz->tNum;i++)
				{
					item["name"] = ptz->name[i];
					item["token"] = ptz->token[i];
					(*paraObj).append(item);
				}
			}
			else
			{
				(*paraObj).resize(0);
			}
		}
		break;
        case IVC_GETNETCONFIG:
        {
            ivtRPCNetConfigR* config = (ivtRPCNetConfigR*)(pStructData->params);
            if(config->net_count)
            {
                for (i=0; i < config->net_count; i++)
                {
                    item["name"] = config->net_config_list[i].name;
                    item["ip"] = config->net_config_list[i].ip;
                    item["dns1"] = config->net_config_list[i].dns1;
                    item["dns2"] = config->net_config_list[i].dns2;
                    item["gateway"] = config->net_config_list[i].gateway;
                    item["netmask"] = config->net_config_list[i].netmask;
                    item["dhcp"] = config->net_config_list[i].dhcp;
                    item["mac"] = config->net_config_list[i].mac;
                    (*paraObj).append(item);
                }
            }
			else
			{
				(*paraObj).resize(0);
			}
        }
        break;
		default:
			IVT_ERR("writeIVTResParams RPC_ELSE.\n");
			return -1;
		break;
	}

	return 0;

}

int ivtRPCJson::writeIVTEventParams(ivtRPCStruct *pStructData, Json::Value *paraObj)
{
    Json::Value arrayObj;
	Json::Value item;
	
	switch(pStructData->subType)
	{
	    /*
		case IVT_KEEPALIVE:
		{
			ivtRPCKeepAlive *liveEvent;
			liveEvent = (ivtRPCKeepAlive *)(pStructData->params);
			(*paraObj)["state"] = liveEvent->state;

			if(liveEvent->chnlNum)
			{
				for(i=0;i<liveEvent->chnlNum;i++)
				{
					item["state"] = liveEvent->chnlState[i];
					item["channel"] = liveEvent->chnlID[i];
					arrayObj.append(item);
				}

				(*paraObj)["channels"] = arrayObj;
			}
			else
			{
				(*paraObj)["channels"].resize(0);
			}
		}
		break;*/
		default:
			IVT_ERR("writeIVTResParams RPC_ELSE.\n");
			return -1;
		break;
	}

	return 0;
}


int ivtRPCJson::ivtStructDataToPayLoad(char *pJsonData,
	                          ivtRPCStruct *pStructData, int jsonDataLen)
{
	Json::Value root;
	Json::Value item;
	int len;
	string strOut;
	Json::FastWriter jWriter(strOut);

	switch(pStructData->rpcType)
	{
		case RPC_REQ:
		{
			root["req"] = m_ivt_methodType[pStructData->subType];
			root["seq"] = pStructData->seq;
			if(!writeIVTReqParams(pStructData, &item))
				root["params"] = item;
		}
		break;
		case RPC_RESP:
		{
			root["seq"] = pStructData->seq;

			if(!writeIVTResParams(pStructData, &item))
				root["resp"] = item;
			else
				root["resp"] = "";
		}
		break;
		case RPC_EVENT:
		{
			root["event"] = m_ivt_eventType[pStructData->subType];
			//need more to set item
			if(!writeIVTEventParams(pStructData, &item))
				root["params"] = item;
		}
		break;
		case RPC_ERR:
		{
			ivtRPCErr *err;
			err = (ivtRPCErr *)(pStructData->params);
			root["seq"] = pStructData->seq;
		    item["code"] = err->errCode;
			item["msg"] = err->msg;
			root["err"] = item;
		}
		break;
		default:
			IVT_ERR("ivtStructDataToPayLoad RPC_ELSE.\n");
			return -1;
		break;
	}
	
	jWriter.write(root);
	len = strOut.length();
	if(jsonDataLen>len)
	{
		strcpy(pJsonData, strOut.c_str());
		return len;
	}

	return -1;
}








































