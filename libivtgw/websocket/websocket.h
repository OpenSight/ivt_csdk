/* 
 * File:   websocket.h
 * Author: sundq
 *
 * Created on 2014-4-1, 10:26
 */
#ifndef WEBSOCKET_H
#define	WEBSOCKET_H

#ifdef WIN32
#include <windows.h>
#include <process.h>
#define MUTEX CRITICAL_SECTION 
#define mutex_init(b) InitializeCriticalSection(&b)
#define mutex_destroy(b) DeleteCriticalSection(&b)
#define mutex_lock(b)  EnterCriticalSection(&b)
#define mutex_unlock(b)  LeaveCriticalSection(&b)
#else
#include <pthread.h>
#define MUTEX pthread_mutex_t
#define mutex_init(b) pthread_mutex_init(&b, NULL)
#define mutex_destroy(b) pthread_mutex_destroy(&b)
#define mutex_lock(b)  pthread_mutex_lock(&b)
#define mutex_unlock(b)  pthread_mutex_unlock(&b)
#endif


#ifdef	__cplusplus
extern "C"
{
#endif

//#define _free(p) do{free(p);p=NULL;}while(0)
#define RCV_BUF_LEN (128*1024)
#define SEND_RCV_FRM_LEN (8*1024)  
#define WS_SEND_TIMEOUT (300)  /*5 minutes */


typedef struct
{
    char scheme[8];
    char path[1024];
    char query[1024];
    char hostname[128];
    unsigned int port;
} url_t;

typedef enum
{
    OPCODE_CONT = 0x0,
    OPCODE_TEXT = 0x1,
    OPCODE_BINARY = 0x2,
    OPCODE_CLOSE = 0x8,
    OPCODE_PING = 0x9,
    OPCODE_PONG = 0xa
} opcode_t;

typedef enum
{
    STATUS_NORMAL = 1000,
    STATUS_GOING_AWAY = 1001,
    STATUS_PROTOCOL_ERROR = 1002,
    STATUS_UNSUPPORTED_DATA_TYPE = 1003,
    STATUS_STATUS_NOT_AVAILABLE = 1005,
    STATUS_ABNORMAL_CLOSED = 1006,
    STATUS_INVALID_PAYLOAD = 1007,
    STATUS_POLICY_VIOLATION = 1008,
    STATUS_MESSAGE_TOO_BIG = 1009,
    STATUS_INVALID_EXTENSION = 1010,
    STATUS_UNEXPECTED_CONDITION = 1011,
    STATUS_TLS_HANDSHAKE_ERROR = 1015
} close_code_t;

typedef enum
{
    //data length threashold.
    LENGTH_7 = 0x7d,
    LENGTH_16 = (1 << 16)
} threshold_t;

typedef struct
{
    int fin;
    int rsv1;
    int rsv2;
    int rsv3;
    int mask;
    int opcode;
    char *data;
    unsigned long long length;
} ANBF_t;

typedef struct
{
    char *recv_buff;
	char *send_buff;
    char *cont_data;
	int socketErr;
    int fd;
    unsigned int cont_data_size;
	MUTEX sendMutex;
	MUTEX gMutex;
	//add a lock
} wsContext_t;

int recvData(wsContext_t *ctx,void *buff, int len);
int sendBinary(wsContext_t *ctx,void *payload, int len);
int sendUtf8Data(wsContext_t *ctx,void *payload, int len);
int sendCloseing(wsContext_t *ctx,unsigned short status, const char *reason);
int sendPing(wsContext_t *ctx,void *payload, int len);
int sendPong(wsContext_t *ctx,void *payload, int len);
int wsCreateConnection(wsContext_t  *ctx, const char *url);
wsContext_t *wsContextNew();
int wsContextFree(wsContext_t *ctx);

#ifdef	__cplusplus
}
#endif

#endif	/* WEBSOCKET_H */

