#include <stdio.h>
#include <winsock2.h>
#include <errno.h>
#include "server.h"
#include "../until/stream.h"
#include "../log.h"
#include "../config.h"
#include "../header.h"

#pragma comment(lib, "ws2_32.lib");
static void socket_recv(SOCKETINFO *psocketinfo);
static void socket_send(SOCKETINFO *psocketinfo);
static int sendall(SOCKET sockfd, char *buf, int len, int flag);
static int sendstream(SOCKET sockfd, stream *s, int flag);
// static void handle(void *psockClient);
static void handle_body(SOCKETINFO *psocketinfo);
// static void handleServerError(SOCKET sockClient);
// static void handleServerNotFound(SOCKET sockClient);

void server_init()
{
    WORD wVersionRequested = MAKEWORD(1, 1);
    WSADATA wsaData;
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        ExitProcess(-1);
    }
    //初始化成功！但版本不一致! 退出函数
    if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
    {
        WSACleanup(); //清除Socket
        ExitProcess(-1);
    }
}
void server_close(void)
{
    //清除Socket
    WSACleanup();
}
void server_listen(char *ip, unsigned short port)
{
    SOCKET sockClient, sockServer;
    SOCKADDR_IN addrServer, addrClient;
    int len = sizeof(SOCKADDR);
    fd_set WriteSet;    // 获取可写性的套接字集合
    fd_set ReadSet;     // 获取可读性的套接字集合
    ULONG NonBlock = 1; // 非阻塞
    SOCKETINFO *socketinfo = malloc(sizeof(SOCKETINFO) * MAX_PENDING_CONNECTS);
    sockServer = socket(AF_INET, SOCK_STREAM, 0); //使用TCP协议
    addrServer.sin_addr.S_un.S_addr = inet_addr(ip);
    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(port);
    bind(sockServer, (SOCKADDR *)&addrServer, sizeof(SOCKADDR));
    if (listen(sockServer, MAX_PENDING_CONNECTS) != 0)
    {
        printf("listen(sock)   failed:   %d\n ", WSAGetLastError());
        ExitProcess(-1);
    }
    if (ioctlsocket(sockServer, FIONBIO, &NonBlock) == SOCKET_ERROR)
    {
        printf("ioctlsocket() failed with error %d\n", WSAGetLastError());
        ExitProcess(-1);
    }
    printf("服务器已启动:\n监听中%s:%d...\n", ip, port);
    while (1)
    {
        int i, Total;
        FD_ZERO(&ReadSet);
        FD_ZERO(&WriteSet);
        FD_SET(sockServer, &ReadSet); // 向ReadSet集合中添加监听套接字sockServer
        for (i = 0; i < MAX_PENDING_CONNECTS; i++)
        {
            if (socketinfo[i].state)
            {
                if (socketinfo[i].state == STATE_RECV)
                    FD_SET(socketinfo[i].sock, &ReadSet);
                if (socketinfo[i].state == STATE_SEND)
                    FD_SET(socketinfo[i].sock, &WriteSet);
            }
        }
        Total = select(0, &ReadSet, &WriteSet, NULL, NULL);
        if (Total == SOCKET_ERROR)
        {
            return;
        }
        else
        {
            int i;
            if (FD_ISSET(sockServer, &ReadSet))
            {
                for (i = 0; i < MAX_PENDING_CONNECTS; i++)
                {
                    if (socketinfo[i].state == 0)
                    {
                        break;
                    }
                }
                if (i == MAX_PENDING_CONNECTS)
                    i = 0;
                memset(&(socketinfo[i]), 0, sizeof(SOCKETINFO));
                socketinfo[i].sock = accept(sockServer, (SOCKADDR *)&addrClient, &len);
                socketinfo[i].state = STATE_RECV;
                log("连接来自(%s:%d)", inet_ntoa(addrClient.sin_addr), ntohs(addrClient.sin_port));
            }
            for (i = 0; i < MAX_PENDING_CONNECTS; i++)
            {
                if (socketinfo[i].state)
                {
                    if (FD_ISSET(socketinfo[i].sock, &ReadSet))
                    { //读取数据
                        socket_recv(&socketinfo[i]);
                        if (socketinfo[i].error)
                        {
                            socketinfo[i].state = 0;
                            header_clear(socketinfo[i].header.header);
                            socketinfo[i].header.header = NULL;
                            stream_clear(socketinfo[i].recv_body.stream);
                            socketinfo[i].recv_body.stream = NULL;
                            closesocket(socketinfo[i].sock);
                            log(errstr[socketinfo[i].error]);
                        }
                    }
                    else if (FD_ISSET(socketinfo[i].sock, &WriteSet))
                    { //发送数据
                        socket_send(&socketinfo[i]);
                        if (socketinfo[i].error)
                        {
                            socketinfo[i].state = 0;
                            log(errstr[socketinfo[i].error]);
                            stream_clear(socketinfo[i].send_buf.stream);
                            socketinfo[i].send_buf.stream = NULL;
                            closesocket(socketinfo[i].sock);
                        }
                        else if (socketinfo[i].send_buf.from == stream_len(socketinfo[i].send_buf.stream))
                        {
                            stream_clear(socketinfo[i].send_buf.stream);
                            socketinfo[i].send_buf.stream = NULL;
                            socketinfo[i].send_buf.from = 0;
                            socketinfo[i].state = STATE_RECV;
                            socketinfo[i].send_buf.stream = NULL;
                        }
                    }
                }
            }
        }
    }
    closesocket(sockServer);
}
static void socket_recv(SOCKETINFO *psocketinfo)
{
    BYTE buf[2048];
    int recv_len;
    if (psocketinfo->state != STATE_RECV && psocketinfo->error)
    {
        return;
    }
    do
    {
        recv_len = recv(psocketinfo->sock, buf, sizeof(buf), 0);
        if (recv_len == 0)
        {
            psocketinfo->error = ERR_NO_CONNECT;
            return;
        }
        else if (recv_len == SOCKET_ERROR)
        {
            psocketinfo->error = ERR_NET;
            return;
        }
        else if (recv_len == EAGAIN)
        {
            return;
        }
        else
        {
            // http接收完毕？flag==4接收完毕否则继续接收http头
            if (psocketinfo->recv_header_buf.flag != 4)
            {
                char sp[] = "\r\n\r\n";
                int i;
                for (i = 0; i < recv_len; i++)
                {
                    psocketinfo->recv_header_buf.buf[psocketinfo->recv_header_buf.length++] = buf[i];
                    if (psocketinfo->recv_header_buf.length == MAX_HEADER_LENGTH)
                    { //http头太长错误
                        psocketinfo->error = ERR_HEADER_TO_LONG;
                        return;
                    }
                    if (buf[i] == sp[psocketinfo->recv_header_buf.flag])
                    {
                        psocketinfo->recv_header_buf.flag++;
                        if (psocketinfo->recv_header_buf.flag == 4)
                        { //http头接收完成
                            int len;
                            char *slen;
                            psocketinfo->recv_header_buf.buf[psocketinfo->recv_header_buf.length++] = 0;
                            psocketinfo->header = parse_header(psocketinfo->recv_header_buf.buf);
                            if (psocketinfo->header.header == NULL)
                            { //无效http头
                                psocketinfo->error = ERR_HEADER_INVALID;
                                return;
                            }
                            slen = getHeader(psocketinfo->header.header, "Content-Length");
                            if (slen == NULL)
                            {
                                psocketinfo->recv_body.need_length = 0;
                            }
                            else
                            {
                                psocketinfo->recv_body.need_length = atoi(slen);
                            }
                            break;
                        }
                    }
                    else
                    {
                        psocketinfo->recv_header_buf.flag = 0;
                    }
                }
                if (psocketinfo->recv_body.stream == NULL && psocketinfo->recv_header_buf.flag == 4)
                {
                    psocketinfo->recv_body.stream = newStream();
                    stream_push(psocketinfo->recv_body.stream, &buf[i], recv_len - i - 1);
                }
            }
            else
            {
                if (psocketinfo->recv_body.stream == NULL)
                {
                    psocketinfo->recv_body.stream = newStream();
                }
                stream_push(psocketinfo->recv_body.stream, buf, recv_len);
            }
            if (stream_len(psocketinfo->recv_body.stream) == psocketinfo->recv_body.need_length)
            { //body接收完毕处理
                // 根据http头处理body
                handle_body(psocketinfo);
                return;
            }
        }
    } while (TRUE);
}
static void socket_send(SOCKETINFO *psocketinfo)
{
    if (psocketinfo->state != STATE_SEND && psocketinfo->error)
        return;
    int err = sendstream(psocketinfo->sock, psocketinfo->send_buf.stream, psocketinfo->send_buf.from);
    if (err == SOCKET_ERROR)
    {
        psocketinfo->error = ERR_NET;
        return;
    }
    psocketinfo->send_buf.from += err;
}

static void handle_body(SOCKETINFO *psocketinfo)
{
    stream *s = newStream();
    char path[256];
    FILE *fp;
    config *con = getConfig();
    http header = psocketinfo->header;
    SOCKET sockClient = psocketinfo->sock;
    strcpy(path, con->rootpath);
    if ((fp = fopen(strcat(path, header.path), "rb")) || (strcmp(strrchr(path, '/'), "/") == 0 && (fp = fopen(strcat(path, con->index), "rb"))))
    {
        char sendBuf[MAX_HEADER_LENGTH]; //发送至客户端的字符串
        char header_buf[] = "HTTP/1.1 200 ok\r\nContent-type: %s\r\nContent-Length: %d\r\n\r\n";
        int i;
        char *mime;
        char *ext;
        do
        {
            i = fread(sendBuf, sizeof(BYTE), sizeof(sendBuf), fp);
            stream_push(s, sendBuf, i);
        } while (i == sizeof(sendBuf));
        fclose(fp);
        ext = strrchr(path, '.');
        if (strcmp(ext, ".js") == 0)
        {
            mime = "application/javascript;charset=utf-8";
        }
        else if (strcmp(ext, ".ico") == 0)
        {
            mime = "image/x-icon";
        }
        else
        {
            mime = "text/html; charset=UTF-8";
        }
        i = sprintf(sendBuf, header_buf, mime, stream_len(s));
        s = stream_unshift(s, sendBuf, i);
        stream_clear(psocketinfo->recv_body.stream);
        psocketinfo->recv_body.stream = NULL;
    }
    else
    {
        char header_buf[] = "HTTP/1.1 404 NOTFOUND\r\nContent-type: text/html\r\nContent-Length: 3\r\n\r\n404";
        stream_push(s, header_buf, sizeof(header_buf) - 1);
        stream_clear(psocketinfo->recv_body.stream);
        // handleServerNotFound(sockClient);
    }
    psocketinfo->send_buf.stream = s;
    header_clear(psocketinfo->header.header);
    psocketinfo->header.header = NULL;
    psocketinfo->recv_header_buf.flag = 0;
    psocketinfo->recv_header_buf.length = 0;
    psocketinfo->send_buf.from = 0;
    psocketinfo->state = STATE_SEND;
}
static int sendall(SOCKET sockfd, char *buf, int len, int flag)
{
    int total = 0, err;
    while (len)
    {
        err = send(sockfd, buf, len, 0);
        if (err == SOCKET_ERROR)
        {
            return SOCKET_ERROR;
        }
        else if (err == EAGAIN)
        {
            return total;
        }
        len -= err;
        buf += err;
        total += err;
    }
    return total;
}
static int sendstream(SOCKET sockfd, stream *s, int from)
{
    unsigned int total = 0, err;
    while (s)
    {
        if (s->length < from)
        {
            s = s->next;
            from -= s->length;
            continue;
        }
        err = sendall(sockfd, s->byte + from, s->length - from, 0);
        from = 0;
        if (err == SOCKET_ERROR)
        {
            return SOCKET_ERROR;
        }
        else if (err == EAGAIN)
        {
            // err = 0;
            return total;
        }
        else
        {
            total += err;
            s = s->next;
        }
    }
    return total;
}
