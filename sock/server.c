#include <stdio.h>
#include <winsock2.h>
#include "server.h"
#include "../until/stream.h"
#include "../log.h"
#include "../config.h"
#include "../header.h"

#pragma comment(lib, "ws2_32.lib");
static inline void socket_recv(SOCKETINFO *psocketinfo);
static inline void socket_send(SOCKETINFO *psocketinfo);
// static void handle(void *psockClient);
static void handle_body(SOCKETINFO *psocketinfo);
// static void handleServerError(SOCKET sockClient);
// static void handleServerNotFound(SOCKET sockClient);
#include "send.c"
#include "recv.c"
static inline SOCKETINFO *newSocketInfo(int n)
{
    SOCKETINFO *s = malloc(sizeof(SOCKETINFO)*n);
    memset(s, 0, sizeof(SOCKETINFO)*n);
    return s;
}
void server_init()
{
    WORD wVersionRequested = MAKEWORD(1, 1);
    WSADATA wsaData;
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        ExitProcess(-1);
    }
    //��ʼ���ɹ������汾��һ��! �˳�����
    if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
    {
        WSACleanup(); //���Socket
        ExitProcess(-1);
    }
}
void server_close(void)
{
    //���Socket
    WSACleanup();
}
void server_listen(char *ip, unsigned short port)
{
    SOCKET sockServer;
    SOCKADDR_IN addrServer;
    fd_set WriteSet;    // ��ȡ��д�Ե��׽��ּ���
    fd_set ReadSet;     // ��ȡ�ɶ��Ե��׽��ּ���
    ULONG NonBlock = 1; // ������
    SOCKETINFO *socketinfo = newSocketInfo(MAX_PENDING_CONNECTS);
    sockServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //ʹ��TCPЭ��
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
    printf("������������:\n������%s:%d...\n", ip, port);
    while (1)
    {
        int i, Total;
        FD_ZERO(&ReadSet);
        FD_ZERO(&WriteSet);
        FD_SET(sockServer, &ReadSet); // ��ReadSet��������Ӽ����׽���sockServer
        for (i = 0; i < MAX_PENDING_CONNECTS; i++)
        {
            if (socketinfo[i].state)
            {
                if (socketinfo[i].state == STATE_RECV)
                    FD_SET(socketinfo[i].sock, &ReadSet);
                else if (socketinfo[i].state == STATE_SEND)
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
                SOCKADDR_IN addrClient;
                SOCKET sockClient;
                int len = sizeof(SOCKADDR);
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
                FD_SET(socketinfo[i].sock, &ReadSet);
                log("��������(%s:%d)=>sock[%d]", inet_ntoa(addrClient.sin_addr), ntohs(addrClient.sin_port), socketinfo[i].sock);
            }
            for (i = 0; i < MAX_PENDING_CONNECTS; i++)
            {
                if (socketinfo[i].state)
                {
                    if (FD_ISSET(socketinfo[i].sock, &ReadSet))
                    { //��ȡ����
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
                    { //��������
                        socket_send(&socketinfo[i]);
                        if (socketinfo[i].error)
                        {
                            socketinfo[i].state = 0;
                            stream_clear(socketinfo[i].send_buf.stream);
                            socketinfo[i].send_buf.stream = NULL;
                            closesocket(socketinfo[i].sock);
                            log(errstr[socketinfo[i].error]);
                        }
                        else if (socketinfo[i].send_buf.from == stream_len(socketinfo[i].send_buf.stream))
                        {
                            stream_clear(socketinfo[i].send_buf.stream);
                            socketinfo[i].send_buf.stream = NULL;
                            socketinfo[i].send_buf.from = 0;
                            socketinfo[i].state = STATE_RECV;
                            socketinfo[i].send_buf.stream = NULL;
                            log("sock[%d]�������", socketinfo[i].sock);
                        }
                    }
                }
            }
        }
    }
    closesocket(sockServer);
}
