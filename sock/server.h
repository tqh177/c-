#ifndef SERVER_H
#define SERVER_H
#include <winsock2.h>
#include "../header.h"
#include "../until/stream.h"

#ifndef MAX_PENDING_CONNECTS
#define MAX_PENDING_CONNECTS 10 //最大连接数
#endif

// 状态码
#define STATE_RECV 1 //接收状态
#define STATE_SEND 2 //发送状态
// 错误码
#define ERR_HEADER_TO_LONG 1 //http头过长
#define ERR_BODY_TO_LONG 2   //body过长
#define ERR_HEADER_INVALID 3 //http头无效
#define ERR_NET 4            //网络错误
#define ERR_NO_CONNECT 5     //断开连接

typedef unsigned char BYTE;
typedef unsigned int size_t;
static char *(errstr[]) = {"", "http头过长", "body超过长度", "http头无效", "网络错误", "连接断开"};

typedef struct
{
    SOCKET sock;
    struct
    {
        unsigned int length;         //已经接收到的http头的长度
        int flag;                    //http头接收flag "\r\n\r\n",flag==4
        BYTE buf[MAX_HEADER_LENGTH]; //http头
    } recv_header_buf;
    union {
        struct
        {
            stream *stream;      //body(注意清理)
            unsigned int from;   //当前发送起始位置
            unsigned int length; //stream的长度（暂时没有用）
        } send_buf;
        struct
        {
            stream *stream;           //body(注意清理)
            unsigned int need_length; //需要接收的总长度
            unsigned int length;      //已经接收到的长度（暂时没有用）
        } recv_body;
    };
    http header; //http头信息
    int error;   //1.http头部过长，2.body过长
    int state;   //接收或发送状态
} SOCKETINFO;

void server_init(void);
void server_close(void);
void server_listen(char *ip, unsigned short port);

#endif