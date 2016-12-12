#ifndef __RTMP_VF__H__
#define __RTMP_VF__H__


#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	int channel;
	int stream;
	int record_type;

	//!按时间回放，开始时间，结束时间
	int st_year;
	int st_month;
	int st_day;
	int st_hour;
	int st_mintute;
	int st_second;

	int ed_year;
	int ed_month;
	int ed_day;
	int ed_hour;
	int ed_mintute;
	int ed_second;

	//!按文件回放时使用
	int iDisk;
	int iPart;
	int iCluster;
}RTMP_PLAY_INFO;

//!FrameType -- 3:audio, 0:p frame, 1: iframe
typedef int (*pfnAddFrameData)(void* handle, const int channel, const int stream, const unsigned char* data, const int len, int FrameType, int rate);
typedef struct
{
	int (*pfnOpenSource)(const RTMP_PLAY_INFO* play_info, pfnAddFrameData pfnAddFrame, void** handle);
	int (*pfnPauseSource)(void* handle, const RTMP_PLAY_INFO* play_info);
	int (*pfnCloseSource)(void* handle, const RTMP_PLAY_INFO* play_info);
}RTMP_LIVE_SOURCE;

int RtmpSetLiveSource(RTMP_LIVE_SOURCE* live_source);
int rtmp_Start(int port);

//如果成功返回句柄phandle，失败返回NULL
void *RtmpPublishLiveOpen(char *url);

//关闭，注意发送是阻塞的，不发送数据的时候才允许关闭
int RtmpPublishLiveClose(void *phandle);

//阻塞发送，注意和关闭的互斥，返回-1表示发送失败，基本上socket已经关闭可回收资源了
int RtmpPublishLiveSendAV(void *phandle, const unsigned char* videoData, const int frameLen, int frmType, int frmRate);



#ifdef __cplusplus
};
#endif

#endif
