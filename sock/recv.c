#include "server.h"
#include "../config.h"

inline void socket_recv(SOCKETINFO *psocketinfo); //���տͻ��˷�������Ϣ��������httpͷ
static void handle(SOCKETINFO *psocketinfo);      //������Ҫ���͸��ͻ��˵���Ϣ,�����뻺����
static FILE *tryIndex(char *path, pageindex *index)
{
    config *con = getConfig();
    FILE *fp;
    int from = strlen(path);
    char tpath[216];
    strcpy(tpath, path);
    while (index)
    {
        strcpy(tpath + from, index->page);
        fp = fopen(tpath, "rb");
        if (fp)
        {
            strcpy(path, tpath);
            return fp;
        }
        else
            index = index->next;
    }
    return NULL;
}
static char *getmime(char *ext, mime *m)
{
    if (ext == NULL)
    {
        ext = "*";
    }
    while (m)
    {
        if (strcmp(ext, m->key) == 0)
        {
            return m->value;
        }
        m = m->next;
    }
    return NULL;
}
static inline void socket_recv(SOCKETINFO *psocketinfo)
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
        else
        {
            // http������ϣ�flag==4������Ϸ����������httpͷ
            if (psocketinfo->recv_header_buf.flag != 4)
            {
                char sp[] = "\r\n\r\n";
                int i;
                for (i = 0; i < recv_len; i++)
                {
                    psocketinfo->recv_header_buf.buf[psocketinfo->recv_header_buf.length++] = buf[i];
                    if (psocketinfo->recv_header_buf.length == MAX_HEADER_LENGTH)
                    { //httpͷ̫������
                        psocketinfo->error = ERR_HEADER_TO_LONG;
                        return;
                    }
                    if (buf[i] == sp[psocketinfo->recv_header_buf.flag])
                    {
                        psocketinfo->recv_header_buf.flag++;
                        if (psocketinfo->recv_header_buf.flag == 4)
                        { //httpͷ�������
                            int len;
                            char *slen;
                            psocketinfo->recv_header_buf.buf[psocketinfo->recv_header_buf.length++] = 0;
                            psocketinfo->header = parse_header(psocketinfo->recv_header_buf.buf);
                            if (psocketinfo->header.header == NULL)
                            { //��Чhttpͷ
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
            { //body������ϴ���
                // ����httpͷ����body
                log("sock[%d]�������", psocketinfo->sock);
                handle(psocketinfo);
                return;
            }
        }
    } while (TRUE);
}
static void handle(SOCKETINFO *psocketinfo)
{
    stream *s = newStream();
    char path[256];
    FILE *fp;
    config *con = getConfig();
    http header = psocketinfo->header;
    SOCKET sockClient = psocketinfo->sock;
    strcpy(path, con->rootpath);
    if ((fp = fopen(strcat(path, header.path), "rb")) || (strcmp(strrchr(path, '/'), "/") == 0 && (fp = tryIndex(path, con->index))))
    {
        char sendBuf[MAX_HEADER_LENGTH]; //�������ͻ��˵��ַ���
        char header_buf[] = "HTTP/1.1 200 ok\r\nContent-type: %s\r\nContent-Length: %d\r\n\r\n";
        int i, total = 0;
        char *mime;
        char *ext;
        do
        {
            i = fread(sendBuf, sizeof(BYTE), sizeof(sendBuf), fp);
            total += i;
            stream_push(s, sendBuf, i);
            if (total > MAX_BODY_LENGTH)
                break;
        } while (i == sizeof(sendBuf));
        fclose(fp);
        ext = strrchr(path, '.');
        if (ext != NULL)
            ext++;
        mime = getmime(ext, con->mime);
        if (mime == NULL)
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
