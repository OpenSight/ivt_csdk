#ifndef HTTPCLIENT_H
#define	HTTPCLIENT_H

#ifdef	__cplusplus
extern "C"
{
#endif
int create_http_client(const char *host, int port, int *fd, int timeOut);
int sendRequest(int fd, const char *jsonBuf, const char *cmd, int jLen);
int getResponse(int fd, char *jsonBuf, int jBufLen);
void close_http_client(int fd);
int getResponseAndPost(int fd, char *jsonBuf, int jBufLen);


#ifdef	__cplusplus
}
#endif

#endif































