#include "websocket.h"
#include "util.h"
#include <time.h>

static url_t *_parse_url(const char *url, url_t *ret)
{
    char buff[256] = {0};
    int iret = sscanf(url, "%7[^://]%*c%*c%*c%127[^:]%*c%d/%1023[^?]%*c%1023s", ret->scheme, ret->hostname, &ret->port, ret->path, ret->query);
    if (2 == iret)
    {
        iret = sscanf(url, "%7[^://]%*c%*c%*c%127[^/]/%1023[^?]%*c%1023s", ret->scheme, ret->hostname, ret->path, ret->query);
        ret->port = 80;
    }
    sprintf(buff, "/%s", ret->path);
    sprintf(ret->path, "%s", buff);
    return ret;
}

static int32_t _recv_line(int32_t fd, char *buff)
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
    }

    return i - 1;
}

static int32_t _validate_headers(int32_t fd, char *key)
{
    int32_t iret = 0;
    char buff[256] = {0};
    uint32_t status = 0;
    char value[256] = {0};
    char result[256] = {0};
    char header_k[256] = {0};
    char header_v[256] = {0};
    char base64str[256] = {0};
    int32_t base64_len = 0;
    uint8_t sha1[20] = {0};

    if (_recv_line(fd, buff) < 0)
    {
        iret = -1;
        goto end;
    }

    sscanf(buff, "%*s%d", &status);
    if (status != 101)
    {
        iret = -1;
        goto end;
    }

    while (strcmp(buff, "\r\n") != 0)
    {
        memset(buff, 0, 256);
        memset(header_k, 0, 256);
        memset(header_v, 0, 256);
        if (_recv_line(fd, buff) < 0)
        {
            iret = -1;
            goto end;
        }
        sscanf(buff, "%256s%256s", header_k, header_v);
        if (strncmp(header_k, "Sec-WebSocket-Accept:", 256) == 0)
        {
            snprintf(result, 256, "%s", header_v);
        }
    }
    snprintf(value, 256, "%s%s", key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    sha1Buff(value, strlen(value), sha1);
    base64_encode(sha1, 20, (uint8_t *) base64str, &base64_len);
    if (strncmp(str2lower(base64str), str2lower(result), 256) != 0)
    {
        iret = -1;
        goto end;
    }

end:
    return iret;
}

static int32_t _handshake(int32_t fd, const char *host, unsigned short port, const char *resource, const char *query)
{
    int offset = 0;
    char header_str[512] = {0};
    offset += sprintf(header_str + offset, "GET %s?%s HTTP/1.1\r\n", resource, query);
    offset += sprintf(header_str + offset, "Upgrade: websocket\r\n");
    offset += sprintf(header_str + offset, "Connection: Upgrade\r\n");
    offset += sprintf(header_str + offset, "Host: %s:%u\r\n", host, port);
    offset += sprintf(header_str + offset, "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n");
    offset += sprintf(header_str + offset, "Sec-WebSocket-Version: 13\r\n\r\n");

    send(fd, header_str, offset, 0);
    return _validate_headers(fd, "x3JJHMbDL1EzLkh9GBhXDw==");
}

static int32_t _create_frame(ANBF_t *frame, int fin, int rsv1, int rsv2, int rsv3, int opcode, int has_mask, void *data, int len)
{
    frame->fin = fin;
    frame->rsv1 = rsv1;
    frame->rsv2 = rsv2;
    frame->rsv3 = rsv3;
    frame->mask = has_mask;
    frame->opcode = opcode;
    frame->data = data;
    frame->length = len;

    return 0;
}

static void *_format_frame(ANBF_t *frame, int32_t *size, char *frame_header)
{
    int offset = 0;
	int i;
	unsigned char mask[4];
	uint32_t temp;
    uint16_t header =
            (frame->fin << 15) |
            (frame->rsv1 << 14) |
            (frame->rsv2 << 13) |
            (frame->rsv3 << 12) |
            (frame->opcode << 8);

    char byteLen = 0;

    if (frame->length < LENGTH_7)
    {
        header |= frame->mask << 7 | (uint8_t) frame->length;
    }
    else if (frame->length < LENGTH_16)
    {
        header |= frame->mask << 7 | 0x7e;
        byteLen = 2;
    }
    else
    {
        header |= frame->mask << 7 | 0x7f;
        byteLen = 8;
    }

    temp = sizeof(header) + byteLen + (uint32_t)(frame->length) + (uint32_t)(frame->mask<<2);
    if(temp > SEND_RCV_FRM_LEN)
		return NULL;

    header = htons(header);
    memcpy(frame_header + offset, &header, sizeof (header));
    offset += sizeof (header);
    if (byteLen == 2)
    {
        uint16_t len = htons((uint16_t) frame->length);
        memcpy(frame_header + offset, &len, sizeof (len));
        offset += sizeof (len);
    }
    else if (byteLen == 8)
    {
        uint64_t len = htonll(frame->length);
        memcpy(frame_header + offset, &len, sizeof (len));
        offset += sizeof (len);
    }

	if(frame->mask)
	{
		unsigned int mask_int;

		srand(time(NULL));
		mask_int = rand();
		memcpy(mask, &mask_int, 4);
		*(frame_header+offset) = mask[0];
		*(frame_header+offset+1) = mask[1];
		*(frame_header+offset+2) = mask[2];
		*(frame_header+offset+3) = mask[3];
		offset+=4;
	}

	for(i=0;i<frame->length;i++)
		*(frame_header+offset+i) = frame->data[i]^mask[i&0x3];

    *size = offset + (uint32_t) (frame->length);

    return frame_header;
}

static void *_ANBFmask(uint32_t mask_key, void *data, int32_t len)
{
    int32_t i = 0;
    uint8_t *_m = (uint8_t *) & mask_key;
    uint8_t *_d = (uint8_t *) data;
    for (; i < len; i++)
    {
        _d[i] ^= _m[i&0x3];
    }
    return _d;
}

static int32_t _recv_restrict(int32_t fd, void *buff, int32_t size)
{
    int32_t offset = 0;
    int iret = 0;
    while (offset < size)
    {
        iret = recv(fd, ((char *) buff) + offset, (int32_t) (size - offset), 0);
        if (iret > 0)
        {
            offset += iret;
        }
        else
        {
            offset = -1;
            break;
        }
    }

    return offset;
}

static int32_t _recv_frame(int32_t fd, ANBF_t *frame, char *payload)
{
    uint8_t b1, b2, fin, rsv1, rsv2, rsv3, opcode, has_mask;
    uint64_t frame_length = 0;
    uint16_t length_data_16 = 0;
    uint64_t length_data_64 = 0;
    uint32_t frame_mask = 0;
    uint8_t length_bits = 0;
    uint8_t frame_header[2] = {0};
    int32_t iret = 0;
    //char *payload = NULL;

    iret = _recv_restrict(fd, &frame_header, 2);
    if (iret < 0)
    {
        goto end;
    }

    b1 = frame_header[0];
    b2 = frame_header[1];
    length_bits = b2 & 0x7f;
    fin = b1 >> 7 & 1;
    rsv1 = b1 >> 6 & 1;
    rsv2 = b1 >> 5 & 1;
    rsv3 = b1 >> 4 & 1;
    opcode = b1 & 0xf;
    has_mask = b2 >> 7 & 1;

    if (length_bits == 0x7e)
    {
        iret = _recv_restrict(fd, &length_data_16, 2);
        if (iret < 0)
        {
            goto end;
        }

        frame_length = ntohs(length_data_16);
    }
    else if (length_bits == 0x7f)
    {
        iret = _recv_restrict(fd, &length_data_64, 8);
        if (iret < 0)
        {
            goto end;
        }

        frame_length = ntohll(length_data_64);
    }
    else
    {
        frame_length = length_bits;
    }

    if(has_mask)
    {
        iret = _recv_restrict(fd, &frame_mask, 4);
        if (iret < 0)
        {
            goto end;
        }
    }

    if(frame_length > 0)
    {
        if(frame_length>SEND_RCV_FRM_LEN)
        {
			goto end;
		}

        iret = _recv_restrict(fd, payload, (int32_t) frame_length);
        if (iret < 0)
        {
            goto end;
        }
    }

    if (has_mask)
    {
        _ANBFmask(frame_mask, payload, (uint32_t) frame_length);
    }

    return _create_frame(frame, fin, rsv1, rsv2, rsv3, opcode, has_mask, payload, (uint32_t) frame_length);

end:
    return -1;
}

static int32_t _send(wsContext_t *ctx, void *payload, int32_t len, int32_t opcode, char *framePtr)
{
    int32_t length = 0;
    int32_t iret = 0;
	int32_t fd = ctx->fd;
    ANBF_t frame = {0};
    char *sendData = NULL;

	mutex_lock(ctx->sendMutex);
    _create_frame(&frame, 1, 0, 0, 0, opcode, 1, payload, len);
    sendData = (char *) _format_frame(&frame, &length, framePtr);
	if(!sendData)
	{
		mutex_unlock(ctx->sendMutex);
		return -1;
	}
    iret = send(fd, sendData, length, 0);
    mutex_unlock(ctx->sendMutex);
	if(iret<0)
	{
		//printf("send socket err\n");
		mutex_lock(ctx->gMutex);		
		ctx->socketErr = 1;
		mutex_unlock(ctx->gMutex);  
	}
    return iret;
}

int32_t sendPing(wsContext_t *ctx, void *payload, int32_t len)
{
    return _send(ctx, payload, len, OPCODE_PING, ctx->send_buff);
}

int32_t sendPong(wsContext_t *ctx, void *payload, int32_t len)
{
    return _send(ctx, payload, len, OPCODE_PONG, ctx->send_buff);
}

int32_t sendCloseing(wsContext_t *ctx, uint16_t status, const char *reason)
{
    char *p = NULL;
    int len = 0;
    char payload[64] = {0};
    status = htons(status);
    p = (char *) &status;
    len = snprintf(payload, 64, "\\x%02x\\x%02x%s", p[0], p[1], reason);
    return _send(ctx, payload, len, OPCODE_CLOSE, ctx->send_buff);
}

int32_t recvData(wsContext_t *ctx, void *buff, int32_t len)
{
    int data_len = -1;
    int iret = -1;
    ANBF_t _frame = {0};
    ANBF_t *frame = &_frame;

    while (1)
    {
        memset(frame, 0, sizeof (ANBF_t));
        iret = _recv_frame(ctx->fd, frame, ctx->recv_buff);
        if (iret < 0)
        {           	
			//printf("recv err\n");
			mutex_lock(ctx->gMutex);
			ctx->socketErr = 1;
			mutex_unlock(ctx->gMutex);  
            goto end;
        }

        if (frame->opcode == OPCODE_TEXT || frame->opcode == OPCODE_BINARY || frame->opcode == OPCODE_CONT)
        {
            if (frame->opcode == OPCODE_CONT && 0 == ctx->cont_data_size)
            {
                goto end;
            }
            else
            {
                uint32_t temp;
				temp = ctx->cont_data_size + (uint32_t)frame->length;
            	if(temp >  RCV_BUF_LEN)
					goto end;
                memcpy(ctx->cont_data + ctx->cont_data_size, frame->data, (uint32_t) frame->length);
                ctx->cont_data_size = temp;
            }

            if (frame->fin)
            {
                data_len = ctx->cont_data_size > len ? 0 : ctx->cont_data_size;
				if(data_len)
                	memcpy(buff, ctx->cont_data, data_len);
                goto end;
            }
        }
        else if (frame->opcode == OPCODE_CLOSE)
        {
            sendCloseing(ctx, STATUS_NORMAL, "");
			if(ctx->fd>=0)
			{
				close(ctx->fd);
				ctx->fd = -1;
			}
            goto end;
        }
        else if (frame->opcode == OPCODE_PING)
        {
            sendPong(ctx, "", 0);
        }
        else
        {
            goto end;
        }
    }

end:
    ctx->cont_data_size = 0;
    return data_len;
}

int32_t sendUtf8Data(wsContext_t *ctx, void *data, int32_t len)
{
    return _send(ctx, data, len, OPCODE_TEXT, ctx->send_buff);
}

int32_t sendBinary(wsContext_t *ctx, void *data, int32_t len)
{
    return _send(ctx, data, len, OPCODE_BINARY, ctx->send_buff);
}

int32_t wsCreateConnection(wsContext_t *ctx, const char *url)
{
    url_t purl ;
    memset(&purl,  0, sizeof(url_t));
    _parse_url(url, &purl);
    ctx->fd = ut_connect(purl.hostname, purl.port, WS_SEND_TIMEOUT);
	if(-1==ctx->fd)
		return -1;

    if(_handshake(ctx->fd, purl.hostname, purl.port, purl.path, purl.query)<0)
		return -1;

    return 0;
}

wsContext_t *wsContextNew()
{
    wsContext_t *ctx = (wsContext_t *) malloc(sizeof(wsContext_t));

	if(ctx)
	{
    	memset(ctx, 0, sizeof (wsContext_t));
        ctx->fd = -1;
		ctx->recv_buff = (char *)malloc(SEND_RCV_FRM_LEN);
		ctx->send_buff = (char *)malloc(SEND_RCV_FRM_LEN);
		if(!ctx->recv_buff || !ctx->send_buff)
		{
			goto ERR;
		}

		ctx->cont_data = (char *)malloc(RCV_BUF_LEN);
		if(!ctx->cont_data)
		{
			goto ERR;
		}

		mutex_init(ctx->sendMutex);
		mutex_init(ctx->gMutex);
		ctx->socketErr = 0;
	}

    return ctx;
ERR:
	wsContextFree(ctx);
    return ctx;
}

int32_t wsContextFree(wsContext_t *ctx)
{
	if(ctx)
	{
		if(ctx->recv_buff)
			free(ctx->recv_buff);

		if(ctx->send_buff)
			free(ctx->send_buff);

	    if(ctx->fd>=0)
	    {
			close(ctx->fd);
			ctx->fd = -1;
	    }

		mutex_destroy(ctx->sendMutex);		
		mutex_destroy(ctx->gMutex);
		free(ctx);
	}

    return 0;
}



