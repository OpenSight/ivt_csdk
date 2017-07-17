#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../ivt/ivtMacro.h"

#define HTTP_SEND_TIMEOUT  1

#define USER_PWORD "KkZvY3VzVmlzaW9uOm5vaXNpVnN1Y29GKg=="
int create_http_client(const char *host, int port, int *fd, int timeOut)
{
	  // Create a socket for the client
    struct sockaddr_in address;
    struct timeval timeout;  
    socklen_t len = sizeof(timeout);  
    int new_fd = 0;
    
    if(host == NULL || port == 0 || fd == NULL){
        goto error;
    }
    
    new_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(new_fd < 0){
        goto error;
    }
    
    //send timeout
    timeout.tv_sec = HTTP_SEND_TIMEOUT;  
    timeout.tv_usec = 0;  
    if (setsockopt(new_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len) < 0) {  
		IVT_ERR("setsockopt err\n");
        perror("setsockopt:");
        goto error;
    }     
    
    //recv timeout
    timeout.tv_sec = timeOut;  
    timeout.tv_usec = 0;  
    if (setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, len) < 0) {  		
		IVT_ERR("setsockopt err\n");
        perror("setsockopt:");
        goto error;
    }     
    

    /*  Name the socket, as agreed with the server.  */
	address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(host);
	address.sin_port = htons(port);

    len = sizeof(address);

    /*  Now connect our socket to the server's socket.  */
    if(connect(new_fd, (struct sockaddr *)&address, len) < 0){
		IVT_ERR("connect err %s:%d\n", host, port);
        perror("connect:");
        goto error;
    }

    *fd = new_fd;

	return 0;

error:
    if(new_fd){
        close(new_fd);
        new_fd = -1;
    }
    return -1;
}

void close_http_client(int fd)
{
	if(fd>=0)
		close(fd);
}
static int http_recv_line(int fd, char *buff, int bufLen)
{
    int32_t i = 0;
    int32_t iret = 0;
    char c = 0;
    while ('\n' != c)
    {
        iret = recv(fd, &c, 1, 0);
        if (iret < 0)
        {
            return iret;
        }
        buff[i++] = c;
		if(i==bufLen)
			break;
    }

    return i - 1;
}

int sendRequest(int fd, const char *jsonBuf, const char *cmd, int jLen)
{

    int offset = 0;
	int iret;
    char header_str[2048];
    offset += sprintf(header_str + offset, "POST /merlin/%s HTTP/1.1\r\n", cmd);
    offset += sprintf(header_str + offset, "Content-Type: application/json\r\n");
    offset += sprintf(header_str + offset, "Content-Length: %d\r\n", jLen);
    offset += sprintf(header_str + offset, "Agent: merlin HTTP agent\r\n");
    offset += sprintf(header_str + offset, "Authorization: Basic %s\r\n", USER_PWORD);
    offset += sprintf(header_str + offset, "Data_Param: version=1.0.0.0&operation="\
		"%s&session=0&sequence=1&unit=DEVICE&id=0\r\n\r\n", cmd);

	if(jLen+offset>=2048)
		return -1;
	
	if(jsonBuf)
    	offset += sprintf(header_str + offset,"%s", jsonBuf);

    iret = send(fd, header_str, offset, 0);

    return iret;
}

int getResponse(int fd, char *jsonBuf, int jBufLen)
{
	int iret = 0;
	char buff[512];
	int status = 0;
	int jLen=0, jRLen;
    char header_n[256];
    char header_p[256];

	memset(buff, 0, 512);
	if (http_recv_line(fd, buff, 512) < 0)
	{
		iret = -1;
		goto end;
	}

	sscanf(buff, "%*s%d", &status);
	if (status != 200)
	{
		iret = -1;
		goto end;
	}

	while(strcmp(buff, "\r\n") != 0)
	{
		memset(buff, 0, 512);
		memset(header_n, 0, 256);
		memset(header_p, 0, 256);
		if(http_recv_line(fd, buff, 512) < 0)
		{
		   iret = -1;
		   goto end;
		}
		sscanf(buff, "%255s%255s", header_n, header_p);
		if (strncmp(header_n, "Content-Length:", 255) == 0)
		{
		   sscanf(header_p, "%d", &jLen);
		}
	}

    //receive json
    if(jLen>0 && jBufLen >= (jLen + 1))
    {
        int offset = 0;
        while(offset < jLen){
            jRLen = recv(fd, jsonBuf + offset, jLen - offset, 0);
            if(jRLen <= 0){
                //error 
                iret = -1;
                break;
            }
            offset += jRLen;            
        }
		
        if(offset == jLen){
            jsonBuf[jLen] = 0; //make sure the string 0-terminated
        }
    }
	else if(jLen)
		iret = -1;
end:
    return iret;
}

int getResponseAndPost(int fd, char *jsonBuf, int jBufLen)
{
	int iret = 0;
	char buff[512];
	int status = 0;
	int jLen=0, jRLen;
    char header_n[256];
    char header_p[256];
	
	memset(buff, 0, 512);
	if(http_recv_line(fd, buff, 512) < 0)
	{
		iret = -1;
		goto end;
	}
	
	//if it is POST, skip reading status
    if(!strstr(buff, "POST")) 
    {
		sscanf(buff, "%*s%d", &status);
		if (status != 200)
		{
			iret = -1;
			goto end;
		}
	}

	while(strcmp(buff, "\r\n") != 0)
	{
		memset(buff, 0, 512);
		memset(header_n, 0, 256);
		memset(header_p, 0, 256);
		if(http_recv_line(fd, buff, 512) < 0)
		{
		   iret = -1;
		   goto end;
		}
		sscanf(buff, "%255s%255s", header_n, header_p);
		if (strncmp(header_n, "Content-Length:", 255) == 0)
		{
		   sscanf(header_p, "%d", &jLen);
		}
	}

    //receive json
    if(jLen>0 && jBufLen >= (jLen + 1))
    {
        int offset = 0;
        while(offset < jLen){
            jRLen = recv(fd, jsonBuf + offset, jLen - offset, 0);
            if(jRLen <= 0){
                //error 
                iret = -1;
                break;
            }
            offset += jRLen;            
        }
		
        if(offset == jLen){
            jsonBuf[jLen] = 0; //make sure the string 0-terminated
        }
    }
	else if(jLen)
		iret = -1;
end:
    return iret;
}








