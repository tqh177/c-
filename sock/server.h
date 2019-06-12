#ifndef SERVER_H
#define SERVER_H
#include <winsock2.h>
#include "../header.h"
#include "../until/stream.h"

#ifndef MAX_PENDING_CONNECTS
#define MAX_PENDING_CONNECTS 10 //���������
#endif

// ״̬��
#define STATE_RECV 1 //����״̬
#define STATE_SEND 2 //����״̬
// ������
#define ERR_HEADER_TO_LONG 1 //httpͷ����
#define ERR_BODY_TO_LONG 2   //body����
#define ERR_HEADER_INVALID 3 //httpͷ��Ч
#define ERR_NET 4            //�������
#define ERR_NO_CONNECT 5     //�Ͽ�����

typedef unsigned char BYTE;
typedef unsigned int size_t;
static char *(errstr[]) = {"", "httpͷ����", "body��������", "httpͷ��Ч", "�������", "���ӶϿ�"};

typedef struct
{
    SOCKET sock;
    struct
    {
        unsigned int length;         //�Ѿ����յ���httpͷ�ĳ���
        int flag;                    //httpͷ����flag "\r\n\r\n",flag==4
        BYTE buf[MAX_HEADER_LENGTH]; //httpͷ
    } recv_header_buf;
    union {
        struct
        {
            stream *stream;      //body(ע������)
            unsigned int from;   //��ǰ������ʼλ��
            unsigned int length; //stream�ĳ��ȣ���ʱû���ã�
        } send_buf;
        struct
        {
            stream *stream;           //body(ע������)
            unsigned int need_length; //��Ҫ���յ��ܳ���
            unsigned int length;      //�Ѿ����յ��ĳ��ȣ���ʱû���ã�
        } recv_body;
    };
    http header; //httpͷ��Ϣ
    int error;   //1.httpͷ��������2.body����
    int state;   //���ջ���״̬
} SOCKETINFO;

void server_init(void);
void server_close(void);
void server_listen(char *ip, unsigned short port);

#endif