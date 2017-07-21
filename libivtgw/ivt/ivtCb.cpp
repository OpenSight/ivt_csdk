#include "../http/httpClient.h"
#include "ivtMacro.h"
#include "ivtStruct.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "ivtRPCJson.h"
#include "../libJson/json.h"
#include <string>
#include "ivtCb.h"
using namespace std;

char http_host[HTTP_HOST_LEN];
int http_port;
char ivtFirmware[FM_STRING_LEN];
int keepErrCnt;
//-------------------------------------------------------------------------------------------
int reboot(void)
{
	int fd=-1;
	char JsonData[512];

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT-5)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, NULL, "Reboot.cgi", 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	close_http_client(fd);
	return 0;
ERR_END:
	close_http_client(fd);
	return -1;
}

int getAlarmPort(void)
{
	int fd=-1, port;
	char JsonData[512];
    bool bRet = false;
	Json::Reader reader;
	Json::Value value;

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, NULL, "GetListenPort.cgi", 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	bRet = reader.parse(JsonData, value);
	if(false==bRet)
	{
		goto ERR_END;
	}

	bRet = value.isMember("TCPPort");
	if(false==bRet)
	{
		IVT_ERR("TCPPort is not exist.\n");
		return -1;
	}
	port = value["TCPPort"].asInt();

	close_http_client(fd);
	return port;

ERR_END:
	close_http_client(fd);
	return -1;
}

int getUrl(char *code, char *pw, char *ip, int *port, int *enable, int *snap, int len)
{
    int fd=-1;
	bool bRet = false;
	char JsonData[512];
	//char *ptr;
	Json::Reader reader;
	Json::Value value;
	Json::Value item;
	string strOut;

	if(!code || !pw ||!ip)
        return -1;

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, NULL, "GetIntetVideoCfg.cgi?Type=QuGuanCloud", 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

    if(getResponse(fd, JsonData, 512)<0)
    {
        IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	bRet = reader.parse(JsonData, value);
	if(false==bRet)
	{
		goto ERR_END;
	}

	bRet = value.isMember("QuGuanCloud");
	if(false==bRet)
	{
		goto ERR_END;
	}
	item = value["QuGuanCloud"];
    //-------------------------------------

	bRet = item.isMember("Code");
	if(false==bRet)
	{
		goto ERR_END;
	}
	strOut = item["Code"].asString();
	if((int)strOut.length()>=len)
	{
		goto ERR_END;
	}
	strcpy(code, strOut.c_str());

    //-------------------------------------

    bRet = item.isMember("Password");
	if(false==bRet)
	{
		goto ERR_END;
	}
	strOut = item["Password"].asString();
	if((int)strOut.length()>=len)
	{
		goto ERR_END;
	}
	strcpy(pw, strOut.c_str());
    //-------------------------------------
   	bRet = item.isMember("ServerIP");
	if(false==bRet)
	{
		goto ERR_END;
	}
	strOut = item["ServerIP"].asString();
	if((int)strOut.length()>=len)
	{
		goto ERR_END;
	}
	//IVT_DEBUG("%s\n", strOut.c_str());
	strcpy(ip, strOut.c_str());
    //----------------------------------------
   	bRet = item.isMember("ServerPort");
	if(false==bRet)
	{
		goto ERR_END;
	}
	*port = item["ServerPort"].asInt();
    //-------------------------------------------
    bRet = item.isMember("enable");
	if(false==bRet)
	{
		goto ERR_END;
	}
	*enable = item["enable"].asInt();
   //-----------------------------------------
   	bRet = item.isMember("SnapEnable");
	if(false==bRet)
	{
		*snap = 1;
	}
	else
		*snap = item["SnapEnable"].asInt();

	close_http_client(fd);
	return 0;
ERR_END:
	close_http_client(fd);
	return -1;
}

int getFirmAndMode(char *firmware, char *mode, int len)
{
    int fd=-1;
	char JsonData[512];
	char *ptr;

	if(!firmware || !mode)
        return -1;

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, NULL, "GetVersion.cgi", 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

    if(getResponse(fd, JsonData, 512)<0)
    {
        IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

    ptr = strstr(JsonData, "Mode");
	if(ptr)
    	sscanf(ptr, "%*s %[^\r\n]31s", mode);
	else
		goto ERR_END;

    ptr = strstr(JsonData, "Svn");
	if(ptr)
    	sscanf(ptr, "%*s %[^\r\n]31s", firmware);
	else
		goto ERR_END;

	//IVT_DEBUG("%s\n", ptr);
	close_http_client(fd);
	return 0;
ERR_END:
	close_http_client(fd);
	return -1;
}


//----------------------------------------ivc--req-------------------------------------------
int ivc_rtmpPublishCb(ivtRPCStruct *para,  ivtRPCStruct *paraOut)
{
	Json::Value root;
	int msStream;
	int fd=-1;
	char JsonData[512];
	string strOut;
	Json::FastWriter jWriter(strOut);
	ivtRPCRtmpPublish *rtmp=(ivtRPCRtmpPublish *)(para->params);
	ivtRPCErr *err = (ivtRPCErr *)paraOut->params;
	msStream = rtmp->qaulity<IVT_HD ? 1:0;

	root["stream"] = msStream;
	root["channel"] = rtmp->channel;
	root["stream_Id"] = rtmp->streamID;
	root["url"] = rtmp->url;
	root["max_bitrate"] = rtmp->max_bitrate;

	jWriter.write(root);
	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT+10)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, strOut.c_str(), "RtmpPublish.cgi", strOut.length())<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

    if(getResponse(fd, JsonData, 512)<0)
    {
        IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	paraOut->rpcType = RPC_RESP;
	paraOut->subType = IVC_RTMPPUBLISH;
	paraOut->seq = para->seq;
	close_http_client(fd);
	return 0;
ERR_END:
	paraOut->rpcType = RPC_ERR;
	paraOut->subType = IVC_RTMPPUBLISH;
	paraOut->seq = para->seq;
	err->errCode = 1;
	strcpy(err->msg, "error");
	close_http_client(fd);
	return -1;
}

int ivc_rtmpStopPublishAll()
{
	Json::Value root;
	int fd=-1;
	char JsonData[512];
	string strOut;
	Json::FastWriter jWriter(strOut);

	root["channel"] = 0;
	root["stream_Id"] = "NULL";
	jWriter.write(root);
	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, strOut.c_str(), "RtmpStopPublish.cgi", strOut.length())<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	close_http_client(fd);
    return 0;
ERR_END:
	close_http_client(fd);
    return -1;

}

int ivc_rtmpStopPublishCb(ivtRPCStruct *para,  ivtRPCStruct *paraOut)
{
	Json::Value root;
	int fd=-1;
	char JsonData[512];
	string strOut;
	Json::FastWriter jWriter(strOut);
	ivtRPCStopPublish *rtmp=(ivtRPCStopPublish *)(para->params);
	ivtRPCErr *err = (ivtRPCErr *)paraOut->params;

	root["channel"] = rtmp->channel;
	root["stream_Id"] = rtmp->streamID;
	jWriter.write(root);
	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, strOut.c_str(), "RtmpStopPublish.cgi", strOut.length())<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	paraOut->rpcType = RPC_RESP;
	paraOut->subType = IVC_RTMPSTOPPUBLISH;
	paraOut->seq = para->seq;
	close_http_client(fd);
    return 0;
ERR_END:
	paraOut->rpcType = RPC_ERR;
	paraOut->subType = IVC_RTMPSTOPPUBLISH;
	paraOut->seq = para->seq;
	err->errCode = 1;
	strcpy(err->msg, "error");
	close_http_client(fd);
    return -1;
}

int ivc_getPtzPresetTourListCb(ivtRPCStruct *para,  ivtRPCStruct *paraOut)
{
	Json::Reader reader;
	Json::Value value;
	Json::Value item;
	Json::Value arrayObj;
	int fd=-1, i;
	char JsonData[512];
	bool bRet = false;
	ivtRPCPTZPreList *ptz=(ivtRPCPTZPreList *)(para->params);
	ivtRPCPTZPreListR *ptzR = (ivtRPCPTZPreListR *)(paraOut->params);
	ivtRPCErr *err = (ivtRPCErr *)paraOut->params;

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}
    sprintf(JsonData, "GetPtzTourCfg.cgi?Ch=%d", ptz->channel);
	if(sendRequest(fd, NULL, JsonData, 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	paraOut->rpcType = RPC_RESP;
	paraOut->subType = IVC_GETPTZPRESETTOURLIST;
	paraOut->seq = para->seq;

	bRet = reader.parse(JsonData, value);
	if(false==bRet)
	{
		goto ERR_END;
	}

	bRet = value.isMember("PtzTour");
	if(false==bRet)
	{
	    ptzR->tNum = 0;
		goto R_END;
	}

	arrayObj = value["PtzTour"];
	ptzR->tNum = arrayObj.size();

	if(ptzR->tNum>IVT_TOKEN_NUM)
		ptzR->tNum = IVT_TOKEN_NUM;

	for(i=0;i<ptzR->tNum;i++)
	{
		int id;
		item = arrayObj[i];
		id = item["TourIndex"].asInt();
		sprintf(ptzR->token[i], "%d", id);
        strcpy(ptzR->name[i],ptzR->token[i]);
	}
R_END:
	close_http_client(fd);
    return 0;
ERR_END:
	paraOut->rpcType = RPC_ERR;
	paraOut->subType = IVC_GETPTZPRESETTOURLIST;
	paraOut->seq = para->seq;
	err->errCode = 1;
	strcpy(err->msg, "error");
	close_http_client(fd);
	return -1;

}

int ivc_getPtzPresetListCb(ivtRPCStruct *para,  ivtRPCStruct *paraOut)
{
	Json::Reader reader;
	Json::Value value;
	Json::Value item;
	Json::Value arrayObj;

	int fd=-1, i;
	char JsonData[512];

	bool bRet = false;
	ivtRPCPTZPreList *ptz=(ivtRPCPTZPreList *)(para->params);
	ivtRPCPTZPreListR *ptzR = (ivtRPCPTZPreListR *)(paraOut->params);
	ivtRPCErr *err = (ivtRPCErr *)paraOut->params;

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

    sprintf(JsonData, "GetPtzPresetCfg.cgi?Ch=%d", ptz->channel);
	IVT_DEBUG("JsonData %s\n", JsonData);
	if(sendRequest(fd, NULL, JsonData, 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}
	IVT_DEBUG("JsonData %s\n", JsonData);

	paraOut->rpcType = RPC_RESP;
	paraOut->subType = IVC_GETPTZPRESETLIST;
	paraOut->seq = para->seq;

	bRet = reader.parse(JsonData, value);
	if(false==bRet)
	{
		goto ERR_END;
	}

	bRet = value.isMember("ptz_preset");
	if(false==bRet)
	{
	    ptzR->tNum = 0;
		goto R_END;
	}

	arrayObj = value["ptz_preset"];
	ptzR->tNum = arrayObj.size();

	if(ptzR->tNum>IVT_TOKEN_NUM)
		ptzR->tNum = IVT_TOKEN_NUM;

	for(i=0;i<ptzR->tNum;i++)
	{
		string strName;
		int id;
		item = arrayObj[i];
		strName = item["PresetName"].asString();
		id = item["PresetId"].asInt();
		strncpy(ptzR->name[i], strName.c_str(), IVT_PTZ_NAME_SIZE);
		ptzR->name[i][IVT_PTZ_NAME_SIZE-1] = 0;
		sprintf(ptzR->token[i], "%d", id);
	}

R_END:
	close_http_client(fd);
	return 0;
ERR_END:
	paraOut->rpcType = RPC_ERR;
	paraOut->subType = IVC_GETPTZPRESETLIST;
	paraOut->seq = para->seq;
	err->errCode = 1;
	strcpy(err->msg, "error");
	close_http_client(fd);
	return -1;
}

int ivc_rebootChannelCb(ivtRPCStruct *para,  ivtRPCStruct *paraOut)
{
	int fd=-1;
	char JsonData[512];
	//ivtRPCRebootChl *pBoot=(ivtRPCRebootChl *)(para->params);
	ivtRPCErr *err = (ivtRPCErr *)paraOut->params;

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, NULL, "Reboot.cgi", 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	paraOut->rpcType = RPC_RESP;
	paraOut->subType = IVC_REBOOTCHANNEL;
	paraOut->seq = para->seq;
	close_http_client(fd);
	return 0;
ERR_END:
	paraOut->rpcType = RPC_ERR;
	paraOut->subType = IVC_REBOOTCHANNEL;
	paraOut->seq = para->seq;
	err->errCode = 1;
	strcpy(err->msg, "error");
	close_http_client(fd);
	return -1;
}

int ivc_startCRCb(ivtRPCStruct *para,  ivtRPCStruct *paraOut)
{
	Json::Value root;
	int msStream;
	int fd=-1;
	char JsonData[512];
	string strOut;
	Json::FastWriter jWriter(strOut);
	ivtRPCStartCR *cr=(ivtRPCStartCR *)(para->params);
	ivtRPCErr *err = (ivtRPCErr *)paraOut->params;
	msStream = cr->qaulity<IVT_HD ? 1:0;

	root["stream"] = msStream;
	root["channel"] = cr->channel;
	root["session_id"] = cr->recSession;
    root["seg_duration"] = cr->seg_duration;
	root["seg_max_size"] = cr->seg_max_size;
	root["seg_max_count"] = cr->seg_max_count;
	root["prerecord_seconds"] = cr->prerecord_seconds;
	root["start_ts"] = cr->start_ts;
	root["cbk_url"] = cr->url;
	root["max_bitrate"] = cr->max_bitrate;

	jWriter.write(root);
	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, strOut.c_str(), "CloudRecordStart.cgi", strOut.length())<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

    if(getResponse(fd, JsonData, 512)<0)
    {
        IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	paraOut->rpcType = RPC_RESP;
	paraOut->subType = IVC_STARTCLOUDRECORD;
	paraOut->seq = para->seq;
	close_http_client(fd);
	return 0;
ERR_END:
	paraOut->rpcType = RPC_ERR;
	paraOut->subType = IVC_STARTCLOUDRECORD;
	paraOut->seq = para->seq;
	err->errCode = 1;
	strcpy(err->msg, "error");
	close_http_client(fd);
	return -1;
}

int ivc_stopCRCb(ivtRPCStruct *para,  ivtRPCStruct *paraOut)
{
	int fd=-1;
	char JsonData[512];
	Json::Value root;
	string strOut;
	Json::FastWriter jWriter(strOut);
	ivtRPCStopCR *cr=(ivtRPCStopCR *)(para->params);
	ivtRPCErr *err = (ivtRPCErr *)paraOut->params;

	root["channel"] = cr->channel;
	root["session_id"] = cr->recSession;
	jWriter.write(root);
	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, strOut.c_str(), "CloudRecordStop.cgi", strOut.length())<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	paraOut->rpcType = RPC_RESP;
	paraOut->subType = IVC_STOPCLOUDRECORD;
	paraOut->seq = para->seq;
	close_http_client(fd);
    return 0;
ERR_END:
	paraOut->rpcType = RPC_ERR;
	paraOut->subType = IVC_STOPCLOUDRECORD;
	paraOut->seq = para->seq;
	err->errCode = 1;
	strcpy(err->msg, "error");
	close_http_client(fd);
    return -1;
}

int ivc_stopCRCbAll()
{
	Json::Value root;
	int fd=-1;
	char JsonData[512];
	string strOut;
	Json::FastWriter jWriter(strOut);

	root["channel"] = 0;
	root["session_id"] = "NULL";
	jWriter.write(root);
	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, strOut.c_str(), "CloudRecordStop.cgi", strOut.length())<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	close_http_client(fd);
    return 0;
ERR_END:
	close_http_client(fd);
    return -1;
}

int ivc_alarmMDCfgCb(ivtRPCStruct *para,  ivtRPCStruct *paraOut)
{
	Json::Value root;
	Json::Value arrayObj;
	Json::Value item;
	Json::Value subItem;
    int i, j, h0, m0, s0, h1, m1, s1;
	int fd=-1;
	char motion[] = "Motion";
	char sector[] = "0 0:0:0-24:0:0";
	char setTime[20];
	char setTime1[20];
	char regionArray[23];
	char JsonData[512];
	string strOut;
	Json::FastWriter jWriter(strOut);

	ivtRPCMDC *mdc = (ivtRPCMDC *)(para->params);
	ivtRPCErr *err = (ivtRPCErr *)paraOut->params;

	for(i=0;i<22;i++)
		regionArray[i] = '1';
	regionArray[22] = 0;

	item["Channel"] = mdc->channel;
	item["Enable"] = mdc->enable;
	item["Level"] = mdc->sensitivity/20+1;
	item["Type"] = motion;
    //EventHandler
	subItem["AlarmOutEn"] = 0;
	subItem["EventLatch"] = mdc->delay;
	subItem["Mail"] = 0;
	subItem["PtzEn"] = 0;
	subItem["RecordEn"] = 0;
	subItem["SnapEn"] = 0;
	item["EventHandler"] = subItem;

	//Region
	subItem.clear();
	for(i=0;i<18;i++)
	{
    	subItem[i] = regionArray;//equal to subItem.append(regionArray);
	}
	item["Region"] = subItem;
    //Sector
	h0 = mdc->start/3600;
	m0 = (mdc->start-3600*h0)/60;
	s0 = mdc->start-3600*h0-60*m0;

	h1 = mdc->end/3600;
	m1 = (mdc->end-3600*h1)/60;
	s1 = mdc->end-3600*h1-60*m1;

	sprintf(setTime, "1 %d:%d:%d-%d:%d:%d", h0, m0, s0, h1, m1, s1);

	if(-1!=mdc->start1)
	{
		h0 = mdc->start1/3600;
		m0 = (mdc->start1-3600*h0)/60;
		s0 = mdc->start1-3600*h0-60*m0;

		h1 = mdc->end1/3600;
		m1 = (mdc->end1-3600*h1)/60;
		s1 = mdc->end1-3600*h1-60*m1;

		sprintf(setTime1, "1 %d:%d:%d-%d:%d:%d", h0, m0, s0, h1, m1, s1);
	}
	else
	{
		strcpy(setTime1, sector);
	}

	for(i=0;i<7;i++)
	{
	    subItem.clear();
		subItem.append(setTime);
		subItem.append(setTime1);
	    for(j=2;j<6;j++)
	    {
	    	subItem.append(sector);
		}
		arrayObj.append(subItem);
	}
	item["Sector"] = arrayObj;
	arrayObj.clear();
	arrayObj.append(item);

	root["AlarmCfg"] = arrayObj;
    jWriter.write(root);

    if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, strOut.c_str(), "SetAlarmCfg.cgi", strOut.length())<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	paraOut->rpcType = RPC_RESP;
	paraOut->subType = IVC_ALARMMOVEDETECTCONFIG;
	paraOut->seq = para->seq;
	close_http_client(fd);
    return 0;
ERR_END:
	paraOut->rpcType = RPC_ERR;
	paraOut->subType = IVC_ALARMMOVEDETECTCONFIG;
	paraOut->seq = para->seq;
	err->errCode = 1;
	strcpy(err->msg, "error");
	close_http_client(fd);
    return -1;
}

int ivc_alarmRectDCfgCb(ivtRPCStruct *para,  ivtRPCStruct *paraOut)
{
	Json::Value root;
	Json::Value arrayObj;
	Json::Value item;
	Json::Value subItem;
	Json::Value tempItem;
    int i, j, h0, m0, s0, h1, m1, s1;
	int fd=-1;
	char rect[] = "Rect";
	char sector[] = "0 0:0:0-24:0:0";
	char setTime[20];
	char setTime1[20];
	//char regionArray[23];
	char JsonData[512];
	string strOut;
	Json::FastWriter jWriter(strOut);

	ivtRPCRectDC *rectDc = (ivtRPCRectDC *)(para->params);
	ivtRPCErr *err = (ivtRPCErr *)paraOut->params;

	//item["AlarmColor"] = 255;
	item["Channel"] = rectDc->channel;
	item["Enable"] = rectDc->enable;
	item["Level"] = rectDc->sensitivity/20+1;
	item["Type"] = rect;
    //EventHandler
	subItem["AlarmOutEn"] = 0;
	subItem["EventLatch"] = rectDc->delay;
	subItem["Mail"] = 0;
	subItem["PtzEn"] = 0;
	subItem["RecordEn"] = 0;
	subItem["SnapEn"] = 0;
	item["EventHandler"] = subItem;

	//RectInfo****************************
	subItem.clear();
	tempItem["x"] = (int)(rectDc->ulx*81.92);
	tempItem["y"] = (int)(rectDc->uly*81.92);
	subItem["LU"]= tempItem;

	tempItem["x"] = (int)(rectDc->dlx*81.92);
	tempItem["y"] = (int)(rectDc->dly*81.92);
	subItem["LD"]= tempItem;

	tempItem["x"] = (int)(rectDc->urx*81.92);
	tempItem["y"] = (int)(rectDc->ury*81.92);
	subItem["RU"]= tempItem;

	tempItem["x"] = (int)(rectDc->drx*81.92);
	tempItem["y"] = (int)(rectDc->dry*81.92);
	subItem["RD"]= tempItem;

	subItem["index"] = 0;
    arrayObj.append(subItem);

	item["RectInfo"] = arrayObj;
	item["RectNum"] = 1;
	//RectInfo****************************

	subItem.clear();

    //Sector
	h0 = rectDc->start/3600;
	m0 = (rectDc->start-3600*h0)/60;
	s0 = rectDc->start-3600*h0-60*m0;

	h1 = rectDc->end/3600;
	m1 = (rectDc->end-3600*h1)/60;
	s1 = rectDc->end-3600*h1-60*m1;

	sprintf(setTime, "1 %d:%d:%d-%d:%d:%d", h0, m0, s0, h1, m1, s1);

	if(-1!=rectDc->start1)
	{
		h0 = rectDc->start1/3600;
		m0 = (rectDc->start1-3600*h0)/60;
		s0 = rectDc->start1-3600*h0-60*m0;

		h1 = rectDc->end1/3600;
		m1 = (rectDc->end1-3600*h1)/60;
		s1 = rectDc->end1-3600*h1-60*m1;

		sprintf(setTime1, "1 %d:%d:%d-%d:%d:%d", h0, m0, s0, h1, m1, s1);
	}
	else
	{
		strcpy(setTime1, sector);
	}

	arrayObj.clear();
	for(i=0;i<7;i++)
	{
	    subItem.clear();
		subItem.append(setTime);
		subItem.append(setTime1);
	    for(j=2;j<6;j++)
	    {
	    	subItem.append(sector);
		}
		arrayObj.append(subItem);
	}
	item["Sector"] = arrayObj;
	arrayObj.clear();
	arrayObj.append(item);

	root["AlarmCfg"] = arrayObj;
//configState
    subItem.clear();
    arrayObj.clear();
	item.clear();

	subItem["Abandon"] = 0;
	subItem["Card"] = 0;
	subItem["ColorDvt"] = 0;
	subItem["CrossLine"] = 0;
	subItem["CrossNet"] = 1;
	subItem["Face"] = 0;
	subItem["Focus"] = 0;
	subItem["Leave"] = 0;
	subItem["Scene"] = 0;
	subItem["Trace"] = 0;
	item["Enabled"] = subItem;
    item["Channel"] = rectDc->channel;
	arrayObj.append(item);
	root["configState"] = arrayObj;

    jWriter.write(root);

    if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, strOut.c_str(), "SetIntDetectCfg.cgi", strOut.length())<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	paraOut->rpcType = RPC_RESP;
	paraOut->subType = IVC_ALARMMOVEDETECTCONFIG;
	paraOut->seq = para->seq;
	close_http_client(fd);
    return 0;
ERR_END:
	paraOut->rpcType = RPC_ERR;
	paraOut->subType = IVC_ALARMMOVEDETECTCONFIG;
	paraOut->seq = para->seq;
	err->errCode = 1;
	strcpy(err->msg, "error");
	close_http_client(fd);
    return -1;
}


int ivc_getNetconfigCb(ivtRPCStruct *para,  ivtRPCStruct *paraOut)
{
    Json::Reader reader;
    Json::Value value;
    Json::Value item;
    Json::Value arrayObj;

    int fd=-1, i;
    char JsonData[512];

    bool bRet = false;
	ivtRPCGeneral *params = (ivtRPCGeneral *)(para->params);
	ivtRPCNetConfigR *netConfigR = (ivtRPCNetConfigR *)(paraOut->params);
    ivtRPCErr *err = (ivtRPCErr *)paraOut->params;

    if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
    {
        IVT_ERR("create_http_client err!\n");
        goto ERR_END;
    }

    sprintf(JsonData, "GetGeneralNetCfg.cgi?Ch=%d", params->channel);
    IVT_DEBUG("JsonData %s\n", JsonData);
    if(sendRequest(fd, NULL, JsonData, 0)<0)
    {
        IVT_ERR("create_http_client err!\n");
        goto ERR_END;
    }

    if(getResponse(fd, JsonData, 512)<0)
    {
        IVT_ERR("getResponse err!\n");
        goto ERR_END;
    }
    IVT_DEBUG("JsonData %s\n", JsonData);

    paraOut->rpcType = RPC_RESP;
    paraOut->subType = IVC_GETNETCONFIG;
    paraOut->seq = para->seq;

    bRet = reader.parse(JsonData, value);
    if(false==bRet)
    {
        goto ERR_END;
    }

    bRet = value.isMember("NetCommon");
    if(false==bRet)
    {
        netConfigR->net_count = 0;
        goto R_END;
    }

    arrayObj = value["NetCommon"];
    netConfigR->net_count = arrayObj.size();

    if(netConfigR->net_count > IVT_NET_COUNT)
        netConfigR->net_count = IVT_NET_COUNT;

    for(i=0; i < netConfigR->net_count; i++)
    {
        item = arrayObj[i];
        if (!item.isObject())
        {
            IVT_ERR("Not of object type, but %d\n", item.type());
            goto ERR_END;
        }
        strncpy(netConfigR->net_config_list[i].name, item["EthName"].asString().c_str(), IVT_ETH_NAME_SIZE);
        netConfigR->net_config_list[i].name[IVT_ETH_NAME_SIZE-1] = 0;
        strncpy(netConfigR->net_config_list[i].ip, item["HostIP"].asString().c_str(), IVT_IPV4_SIZE);
        netConfigR->net_config_list[i].ip[IVT_IPV4_SIZE-1] = 0;
        strncpy(netConfigR->net_config_list[i].dns1, item["Dns1"].asString().c_str(), IVT_IPV4_SIZE);
        netConfigR->net_config_list[i].dns1[IVT_IPV4_SIZE-1] = 0;
        strncpy(netConfigR->net_config_list[i].dns2, item["Dns2"].asString().c_str(), IVT_IPV4_SIZE);
        netConfigR->net_config_list[i].dns2[IVT_IPV4_SIZE-1] = 0;
        strncpy(netConfigR->net_config_list[i].gateway, item["GateWay"].asString().c_str(), IVT_IPV4_SIZE);
        netConfigR->net_config_list[i].gateway[IVT_IPV4_SIZE-1] = 0;
        strncpy(netConfigR->net_config_list[i].netmask, item["SubMask"].asString().c_str(), IVT_IPV4_SIZE);
        netConfigR->net_config_list[i].gateway[IVT_IPV4_SIZE-1] = 0;
        strncpy(netConfigR->net_config_list[i].mac, item["MAC"].asString().c_str(), IVT_MAC_SIZE);
        netConfigR->net_config_list[i].mac[IVT_MAC_SIZE-1] = 0;
        if (item["Dhcp"].asInt() == 0)
            netConfigR->net_config_list[i].dhcp = false;
        else
            netConfigR->net_config_list[i].dhcp = true;
    }

    R_END:
    close_http_client(fd);
    return 0;
    ERR_END:
    paraOut->rpcType = RPC_ERR;
    paraOut->subType = IVC_GETNETCONFIG;
    paraOut->seq = para->seq;
    err->errCode = 1;
    strcpy(err->msg, "error");
    close_http_client(fd);
    return -1;
}

int ivc_getRtmpPublishConfigCb(ivtRPCStruct *para,  ivtRPCStruct *paraOut)
{
    Json::Reader reader;
    Json::Value value;
    Json::Value item;
    Json::Value arrayObj;

    int fd=-1, i;
    char JsonData[512];

    bool bRet = false;
    ivtRPCGetRtmpPulibshConfigR *ConfigR = (ivtRPCGetRtmpPulibshConfigR *)(paraOut->params);
    ivtRPCErr *err = (ivtRPCErr *)paraOut->params;

    if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
    {
        IVT_ERR("create_http_client err!\n");
        goto ERR_END;
    }

    sprintf(JsonData, "GetXvrConfig2.cgi?name=RtmpPublish");
    IVT_DEBUG("JsonData %s\n", JsonData);
    if(sendRequest(fd, NULL, JsonData, 0)<0)
    {
        IVT_ERR("create_http_client err!\n");
        goto ERR_END;
    }

    if(getResponse(fd, JsonData, 4096)<0)
    {
        IVT_ERR("getResponse err!\n");
        goto ERR_END;
    }
    IVT_DEBUG("JsonData %s\n", JsonData);

    paraOut->rpcType = RPC_RESP;
    paraOut->subType = IVC_GETRTMPPUBLISHCONFIG;
    paraOut->seq = para->seq;

    bRet = reader.parse(JsonData, value);
    if(false==bRet)
    {
        goto ERR_END;
    }

    bRet = value.isMember("RtmpPublish");
    if(false==bRet)
    {
        ConfigR->config_count = 0;
        goto R_END;
    }

    arrayObj = value["RtmpPublish"];
    ConfigR->config_count = arrayObj.size();

    if(ConfigR->config_count > IVT_RTMP_PUBLISH_CONFIG_COUNT)
        ConfigR->config_count = IVT_RTMP_PUBLISH_CONFIG_COUNT;

    for(i=0; i < ConfigR->config_count; i++)
    {
        item = arrayObj[i];
        ConfigR->config_list[i].channel = item["Channel"].asInt();
        ConfigR->config_list[i].quality = item["Stream"].asInt()>0?IVT_SD:IVT_HD;
        if (item["Enable"].asInt() == 0)
            ConfigR->config_list[i].enable = false;
        else
            ConfigR->config_list[i].enable = true;
        strncpy(ConfigR->config_list[i].url, item["Url"].asString().c_str(), IVT_URL_SIZE);
        ConfigR->config_list[i].url[IVT_URL_SIZE-1] = 0;
    }

    R_END:
    close_http_client(fd);
    return 0;
    ERR_END:
    paraOut->rpcType = RPC_ERR;
    paraOut->subType = IVC_GETRTMPPUBLISHCONFIG;
    paraOut->seq = para->seq;
    err->errCode = 1;
    strcpy(err->msg, "error");
    close_http_client(fd);
    return -1;
}

int ivc_setRtmpPublishConfigCb(ivtRPCStruct *para,  ivtRPCStruct *paraOut)
{
    Json::Value root;
    Json::Value rtmpPublish;

    string strOut;
    Json::FastWriter jWriter(strOut);
    Json::Value item;

    int fd=-1, i;
    char JsonData[512];

    ivtRPCSetRtmpPulibshConfig *params = (ivtRPCSetRtmpPulibshConfig *)(para->params);
    ivtRPCErr *err = (ivtRPCErr *)paraOut->params;

    if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
    {
        IVT_ERR("create_http_client err!\n");
        goto ERR_END;
    }

    for (i=0; i<params->config_count; i++)
    {
        item["Enable"] = params->config_list[i].enable?1:0;
        item["Channel"] = params->config_list[i].channel;
        item["Stream"] = params->config_list[i].quality<IVT_HD ? 1:0;
        item["Url"] = params->config_list[i].url;
        rtmpPublish.append(item);
    }
    root["RtmpPublish"] = rtmpPublish;
    jWriter.write(root);

    sprintf(JsonData, "SetXvrConfig2.cgi");
    IVT_DEBUG("CMD %s, JsonData %s\n", JsonData, strOut.c_str());
    if(sendRequest(fd, strOut.c_str(), JsonData, strOut.length())<0)
    {
        IVT_ERR("create_http_client err!\n");
        goto ERR_END;
    }

    if(getResponse(fd, JsonData, 512)<0)
    {
        IVT_ERR("getResponse err!\n");
        goto ERR_END;
    }
    IVT_DEBUG("JsonData %s\n", JsonData);

    paraOut->rpcType = RPC_RESP;
    paraOut->subType = IVC_SETRTMPPUBLISHCONFIG;
    paraOut->seq = para->seq;
    close_http_client(fd);
    return 0;

    ERR_END:
    paraOut->rpcType = RPC_ERR;
    paraOut->subType = IVC_SETRTMPPUBLISHCONFIG;
    paraOut->seq = para->seq;
    err->errCode = 1;
    strcpy(err->msg, "error");
    close_http_client(fd);
    return -1;
}

//--------------------------------ivc event-----------------------------------------------------
int ivc_ctrPtzCb(ivtRPCStruct *para)
{
	int fd=-1;
	char JsonData[512];
    int speed;
	ivtRPCPtzCtrl *ptz=(ivtRPCPtzCtrl *)(para->params);

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}
	speed = 1+ ptz->speed/12;
	if(speed>8)
		speed = 8;

    sprintf(JsonData, "PtzCtrl.cgi?operation=%d&speed=%d&channelno=%d",
		                      ptz->opType, speed, ptz->channel);

	IVT_DEBUG("JsonData %s\n", JsonData);
	if(sendRequest(fd, NULL, JsonData, 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	IVT_DEBUG("JsonData %s\n", JsonData);

	close_http_client(fd);
	return 0;
ERR_END:
	close_http_client(fd);
	return -1;
}

int ivc_gotoPtzPresetCb(ivtRPCStruct *para)
{
	int fd=-1;
	char JsonData[512];
	int token;
	ivtRPCGotoPTZPreset *ptz=(ivtRPCGotoPTZPreset *)(para->params);

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	token = atoi(ptz->token);//if token is string but not zero , 0 ascii 48
	if(!token && 48!=ptz->token[0])//0 ascii 48
		goto ERR_END;

	sprintf(JsonData, "PtzCtrl.cgi?operation=20&speed=5&channelno=%d&value=%d",
							 ptz->channel, token);

	IVT_DEBUG("JsonData %s\n", JsonData);
	if(sendRequest(fd, NULL, JsonData, 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}
	IVT_DEBUG("JsonData %s\n", JsonData);

	close_http_client(fd);
	return 0;
ERR_END:
	close_http_client(fd);
	return -1;
}

int ivc_ctrlPtzPresetTourCb(ivtRPCStruct *para)
{
	int fd=-1;
	char JsonData[512];
	int token, opType;
	ivtRPCCtrlPTZPresetTour *ptz=(ivtRPCCtrlPTZPresetTour *)(para->params);

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	token = atoi(ptz->token);//if token is string but not zero , 0 ascii 48
	if(!token && 48!=ptz->token[0])//0 ascii 48
		goto ERR_END;
    opType = ptz->opType+29;
	sprintf(JsonData, "PtzCtrl.cgi?operation=%d&speed=5&channelno=%d&value=%d",
							 opType, ptz->channel, token);

	IVT_DEBUG("JsonData %s\n", JsonData);
	if(sendRequest(fd, NULL, JsonData, 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}
	IVT_DEBUG("JsonData %s\n", JsonData);

	close_http_client(fd);
	return 0;
ERR_END:
	close_http_client(fd);
	return -1;
}

int ivc_ctrlPtzPatrolCb(ivtRPCStruct *para)
{
	int fd=-1;
	char JsonData[512];
	int /*token,*/ opType;
	ivtRPCCtrlPTZPatrol *ptz=(ivtRPCCtrlPTZPatrol *)(para->params);

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	opType = ptz->opType+33; //token set to 0 ????????????????????
	sprintf(JsonData, "PtzCtrl.cgi?operation=%d&speed=5&channelno=%d&value=0",
							 opType, ptz->channel);

	IVT_DEBUG("JsonData %s\n", JsonData);
	if(sendRequest(fd, NULL, JsonData, 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}
	IVT_DEBUG("JsonData %s\n", JsonData);

	close_http_client(fd);
	return 0;
ERR_END:
	close_http_client(fd);
	return -1;
}

int ivc_syncTimeCb(ivtRPCStruct *para)
{
	int fd = -1;
	char JsonData[512];
	tm tmp_time;
	unsigned int dateTimeInSec;
	Json::Reader reader;
	Json::Value value;
	Json::Value item;
	bool bRet = false;

	ivtRPCSyncTime *synTime=(ivtRPCSyncTime *)(para->params);

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(sendRequest(fd, NULL, "GetTimeCfg.cgi", 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

    if(getResponse(fd, JsonData, 512)<0)
    {
        IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	IVT_DEBUG("JsonData %s\n", JsonData);

	close_http_client(fd); //check return  is right
    fd = -1;

	bRet = reader.parse(JsonData, value);
	if(false==bRet)
	{
		goto ERR_END;
	}

	bRet = value.isMember("TimeCfg");
	if(false==bRet)
	{
		goto ERR_END;
	}
	item = value["TimeCfg"];

    tmp_time.tm_isdst = 0;
	bRet = item.isMember("day");
	if(false==bRet)
	{
		IVT_ERR("day is not exist.\n");
		goto ERR_END;;
	}
	tmp_time.tm_mday = item["day"].asInt();

	bRet = item.isMember("hour");
	if(false==bRet)
	{
		IVT_ERR("hour is not exist.\n");
		goto ERR_END;;
	}
	tmp_time.tm_hour = item["hour"].asInt();


	bRet = item.isMember("minute");
	if(false==bRet)
	{
		IVT_ERR("minute is not exist.\n");
		goto ERR_END;;
	}
	tmp_time.tm_min = item["minute"].asInt();

	bRet = item.isMember("month");
	if(false==bRet)
	{
		IVT_ERR("month is not exist.\n");
		goto ERR_END;;
	}
	tmp_time.tm_mon = item["month"].asInt();

	bRet = item.isMember("second");
	if(false==bRet)
	{
		IVT_ERR("second is not exist.\n");
		goto ERR_END;;
	}
	tmp_time.tm_sec = item["second"].asInt();

	bRet = item.isMember("year");
	if(false==bRet)
	{
		IVT_ERR("year is not exist.\n");
		goto ERR_END;;
	}

	tmp_time.tm_year = item["year"].asInt()-1900;
	dateTimeInSec = mktime(&tmp_time);
	//IVT_DEBUG("1 dateTimeInSec %d %d.\n", synTime->dateTimeInSec, dateTimeInSec);

	if(dateTimeInSec>(synTime->dateTimeInSec+synTime->offsetSec) ||
		dateTimeInSec<(synTime->dateTimeInSec-synTime->offsetSec))
	{
		string strOut;
		Json::FastWriter jWriter(strOut);

		if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
		{
			IVT_ERR("create_http_client err!\n");
			goto ERR_END;
		}

		value.clear();
		item.clear();
		item["day"] = synTime->day;
		item["hour"] = synTime->hour;
		item["minute"] = synTime->minute;
		item["month"] = synTime->mon;
		item["second"] = synTime->sec;
		item["year"] = synTime->year;
		value["TimeCfg"] = item;
		jWriter.write(value);

		if(sendRequest(fd, strOut.c_str(), "SetTimeCfg.cgi", strOut.length())<0)
		{
			IVT_ERR("create_http_client err!\n");
			goto ERR_END;
		}

		if(getResponse(fd, JsonData, 512)<0)
		{
		    IVT_ERR("getResponse err!\n");
			goto ERR_END;
		}

		close_http_client(fd);
	}

	return 0;
ERR_END:
	close_http_client(fd);
	return -1;
}

//-----------------------------------------ivt resp-------------------------------------------------

int ivt_alarmNotifyCb(ivtRPCStruct *para)
{
	return -1;
/*
	int fd=-1;
	char JsonData[512];
	Json::Value root;
	string strOut;
	Json::FastWriter jWriter(strOut);
	ivcRPCAlarmRes *res=(ivcRPCAlarmRes *)(para->params);

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	root["channel"] = res->channel;
	root["url"] = res->url;
	jWriter.write(root);
	if(sendRequest(fd, strOut.c_str(), "SnapshotPush.cgi", strOut.length())<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	close_http_client(fd);
	return 0;
ERR_END:
	close_http_client(fd);
	return -1;*/
}


int ivt_previewServerCb(ivtRPCStruct *para)
{
	int fd=-1;
	char JsonData[512];
	Json::Value root;
	string strOut;
	Json::FastWriter jWriter(strOut);
	ivcRPCPreviewRes *res=(ivcRPCPreviewRes *)(para->params);

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	root["channel"] = 0;
	root["url"] = res->url;
	jWriter.write(root);
	if(sendRequest(fd, strOut.c_str(), "SnapshotPush.cgi", strOut.length())<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	close_http_client(fd);
	return 0;
ERR_END:
	close_http_client(fd);
	return -1;
}

int ivt_getFirmwareCb(ivtRPCStruct *para)
{
	int fd=-1;
	char JsonData[512];
	Json::Value root;
	string strOut;
	Json::FastWriter jWriter(strOut);
	ivcRPCFirmwareRes *res=(ivcRPCFirmwareRes *)(para->params);

    if(!strlen(res->version) || strncmp(res->url, "http", 4))
		return 0;

	if((!res->forceToUpd) && (!strcmp(res->version, ivtFirmware)))
		return 0;

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	root["firmware_model"] = res->version;
	root["url"] = res->url;
	jWriter.write(root);
	if(sendRequest(fd, strOut.c_str(), "UpdateFirmware.cgi", strOut.length())<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END;
	}

	close_http_client(fd);
	return 0;
ERR_END:
	close_http_client(fd);
	return -1;
}

//-------------------------------------------ivt event------------------------------------------
int ivt_keepAliveCb(ivtRPCStruct *para)
{
	Json::Reader reader;
	Json::Value value;
	Json::Value item;
	Json::Value arrayObj;

	int fd=-1, i, j, chnlNum;
	char JsonData[512];

	bool bRet = false;
	ivtRPCKeepAlive *keepAlive=(ivtRPCKeepAlive *)(para->params);

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT-5)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END_REBOOT;
	}

	if(sendRequest(fd, NULL, "RtmpGetState.cgi", 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END_REBOOT;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
	    IVT_ERR("getResponse err!\n");
		goto ERR_END_REBOOT;
	}
    keepErrCnt = 0;
	bRet = reader.parse(JsonData, value);
	if(false==bRet)
	{
		goto ERR_END;
	}

	bRet = value.isMember("state");
	if(false==bRet)
	{
		goto ERR_END;
	}

	keepAlive->state = value["state"].asInt();

	bRet = value.isMember("channels");
	if(false==bRet)
	{
		goto ERR_END;
	}

	arrayObj = value["channels"];
	keepAlive->chnlNum = arrayObj.size();

	if(keepAlive->chnlNum>IVT_CHANNEL_NUM)
		keepAlive->chnlNum = IVT_CHANNEL_NUM;

	for(i=0;i<keepAlive->chnlNum;i++)
	{
		item = arrayObj[i];
		keepAlive->chnlID[i]= item["channel"].asInt();
		keepAlive->chnlState[i]= item["state"].asInt();
	}

	close_http_client(fd);
    fd = -1;

	if(create_http_client(http_host, http_port, &fd, HTTP_RECV_TIMEOUT-5)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END_REBOOT;
	}

	if(sendRequest(fd, NULL, "CloudRecordGetState.cgi", 0)<0)
	{
		IVT_ERR("create_http_client err!\n");
		goto ERR_END_REBOOT;
	}

	if(getResponse(fd, JsonData, 512)<0)
	{
		IVT_ERR("getResponse err!\n");
		goto ERR_END_REBOOT;
	}

    keepErrCnt = 0;
	bRet = reader.parse(JsonData, value);
	if(false==bRet)
	{
		goto ERR_END;
	}

	bRet = value.isMember("idList");
	if(false==bRet)
	{
		//set to 0
		for(j=0;j<keepAlive->chnlNum;j++)
		{
			keepAlive->recSession[j][0] = 0;
		}
		close_http_client(fd);
		return 0;
	}

	arrayObj = value["idList"];
	chnlNum = arrayObj.size();

	if(chnlNum>IVT_CHANNEL_NUM)
		chnlNum = IVT_CHANNEL_NUM;

	for(i=0;i<chnlNum;i++)
	{
		int chnlID;
		item = arrayObj[i];
		chnlID = item["channel"].asInt();
		for(j=0;j<keepAlive->chnlNum;j++)
		{
			if(chnlID == keepAlive->chnlID[j])
			{
				string strTemp;
				strTemp = item["session_id"].asString();
				strncpy(keepAlive->recSession[j], strTemp.c_str(), IVT_RCD_ID_SIZE);
				keepAlive->recSession[j][IVT_RCD_ID_SIZE-1] = 0;
			}
		}
	}

	close_http_client(fd);
	return 0;
ERR_END_REBOOT:
	keepErrCnt++;
	if(20==keepErrCnt) //5 minutes
	{
		IVT_DEBUG("********************reboot!*******************\n");
		sleep(1);
		if(reboot()<0)
		{
        	system("reboot");
		}
	}
ERR_END:
	IVT_DEBUG("ERR_END:set state to upgrading %d!\n", keepErrCnt);
	keepAlive->state = 3;
	keepAlive->chnlNum = 1;
	keepAlive->chnlID[0] = 0;
	keepAlive->chnlState[0] = 0;
	keepAlive->recSession[0][0] = 0;
	close_http_client(fd);
	return -1;
}

//-------------------------alarm--------------------------------------------------------
int ivt_recvAlarmData(ivtRPCAlarmNotify *alarm, int fd)
{
	char alarmData[768];
	char *ptr;
    int retVal = -1;

	if(getResponseAndPost(fd, alarmData, 768)<0)
	{
		IVT_ERR("getResponse err!\n");
		retVal = -2; //http connection err, reset http client.
		goto ERR_END;
	}

	if('{'!=alarmData[0])
	{
		ptr = strstr(alarmData, "type=");
		if(ptr)
			sscanf(ptr, "type=%d&", &alarm->type);
		else
			goto ERR_END;

		if(10==alarm->type)
			alarm->type = IVT_ALARM_RECT;

		ptr = strstr(alarmData, "status=");
		if(ptr)
			sscanf(ptr, "status=%1d", &alarm->state);
		else
			goto ERR_END;

		alarm->state -= 1;
		if(0!=alarm->state && 1!=alarm->state)
		{
			goto ERR_END;
		}

		ptr = strstr(alarmData, "channelno=");
		if(ptr)
			sscanf(ptr, "channelno=%1d", &alarm->channel);
		else
			goto ERR_END;

		if(alarm->channel>=IVT_CHANNEL_NUM)
			goto ERR_END;
	}
	else //json
	{
		Json::Reader reader;
		Json::Value value;
		Json::Value item;
		Json::Value subItem;
		string strTemp;
		int size;
		bool bRet = false;

		//IVT_DEBUG("alarmData %s\n", alarmData);
		alarm->type = IVT_ALARM_HEARTBEAT;
		bRet = reader.parse(alarmData, value);
		if(false==bRet)
		{
			goto ERR_END;
		}

		bRet = value.isMember("AlarmState");
		if(false==bRet)
		{
			goto ERR_END;
		}
        item = value["AlarmState"];

		bRet = item.isMember("alarm");
		if(false==bRet)
		{
			memset(alarm->outAlarmState, '0', IVT_CHANNEL_NUM);
		}
		else
		{
			strTemp = item["alarm"].asString();

			size = strTemp.length();
			if(size>=IVT_CHANNEL_NUM)
			{
				memcpy(alarm->outAlarmState, strTemp.c_str(), IVT_CHANNEL_NUM);
			}
			else
			{
				memset(alarm->outAlarmState, '0', IVT_CHANNEL_NUM);
				memcpy(alarm->outAlarmState, strTemp.c_str(), size);
			}
		}

		bRet = item.isMember("motion");
		if(false==bRet)
		{
			memset(alarm->motionState, '0', IVT_CHANNEL_NUM);
		}
		else
		{
			strTemp = item["motion"].asString();

			size = strTemp.length();
			if(size>=IVT_CHANNEL_NUM)
			{
				memcpy(alarm->motionState, strTemp.c_str(), IVT_CHANNEL_NUM);
			}
			else
			{
				memset(alarm->motionState, '0', IVT_CHANNEL_NUM);
				memcpy(alarm->motionState, strTemp.c_str(), size);
			}
		}

		bRet = item.isMember("cross");
		if(false==bRet)
		{
			memset(alarm->rectState, '0', IVT_CHANNEL_NUM);
		}
		else
		{
		    subItem = item["cross"];

			bRet = subItem.isMember("state");
			if(false==bRet)
			{
				memset(alarm->rectState, '0', IVT_CHANNEL_NUM);
			}
			else
			{
				strTemp = subItem["state"].asString();

				size = strTemp.length();
				if(size>=IVT_CHANNEL_NUM)
				{
					memcpy(alarm->rectState, strTemp.c_str(), IVT_CHANNEL_NUM);
				}
				else
				{
					memset(alarm->rectState, '0', IVT_CHANNEL_NUM);
					memcpy(alarm->rectState, strTemp.c_str(), size);
				}
			}
		}

        /*
		bRet = item.isMember("loss");
		if(false==bRet)
		{
			IVT_ERR("loss is not exist.\n");
			goto ERR_END;;
		}
		strTemp = item["loss"].asString();

		size = strTemp.length();
		if(size>=IVT_CHANNEL_NUM)
		{
			memcpy(alarm->lossState, strTemp.c_str(), IVT_CHANNEL_NUM);
		}
		else
		{
			memset(alarm->lossState, 0, size);
			memcpy(alarm->lossState, strTemp.c_str(), size);
		}

		bRet = item.isMember("blind");
		if(false==bRet)
		{
			IVT_ERR("blind is not exist.\n");
			goto ERR_END;;
		}
		strTemp = item["blind"].asString();

		size = strTemp.length();
		if(size>=IVT_CHANNEL_NUM)
		{
			memcpy(alarm->blindState, strTemp.c_str(), IVT_CHANNEL_NUM);
		}
		else
		{
			memset(alarm->blindState, 0, size);
			memcpy(alarm->blindState, strTemp.c_str(), size);
		}*/

	}

	alarm->desc[0] = 'o';
	alarm->desc[1] = 'k';
	alarm->desc[2] = 0;
    return 0;
ERR_END:
	alarm->channel = 0;
	alarm->desc[0] = 'e';
	alarm->desc[1] = 'r';
	alarm->desc[2] = 'r';
	alarm->desc[3] = 0;
	alarm->state = 1;
	alarm->type = IVT_ALARM_UNKNOWN;
	return retVal;
}

int ivt_sendAlarmHeartBeat(int fd)
{
 	if(sendRequest(fd, NULL, "BookAlarm.cgi", 0)<0)
	{
		IVT_ERR("ivt_sendAlarmHeartBeat err!\n");
		goto ERR_END;
	}

	return 0;
ERR_END:
	return -2;
}

//-------------------------------------------------------------------------------
ivcCallBack ivcReqCb[IVC_ELSE_METHOD] = {ivc_rtmpPublishCb, ivc_rtmpStopPublishCb, ivc_rebootChannelCb,
	                                        ivc_getPtzPresetListCb, ivc_getPtzPresetTourListCb,
	                                                ivc_startCRCb, ivc_stopCRCb, ivc_alarmMDCfgCb,
	                                                ivc_alarmRectDCfgCb, ivc_getNetconfigCb,
                                         ivc_getRtmpPublishConfigCb, ivc_setRtmpPublishConfigCb};
ivtCallBack ivtEventCb[IVT_ELSE_EVENT] = {NULL};
ivtCallBack ivcEventCb[IVC_ELSE_EVENT] = {ivc_ctrPtzCb, ivc_gotoPtzPresetCb, ivc_ctrlPtzPresetTourCb,
	                                              ivc_ctrlPtzPatrolCb, ivc_syncTimeCb};
ivtCallBack ivtResCb[IVT_ELSE_METHOD] = {ivt_previewServerCb, ivt_getFirmwareCb, ivt_keepAliveCb, ivt_alarmNotifyCb};


//--------------------------------------------------------------------------------------------


















































