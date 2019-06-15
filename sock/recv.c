#include "server.h"
#include "../config.h"
#include "../api/api.h"
#include <stdio.h>
#include <sys/stat.h>
inline void socket_recv(SOCKETINFO *psocketinfo); //���տͻ��˷�������Ϣ��������httpͷ
static void handle(SOCKETINFO *psocketinfo);      //������Ҫ���͸��ͻ��˵���Ϣ,�����뻺����
// ���ݽ��պ���
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
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                return;
            }
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
                logHttp();
                handle(psocketinfo);
                return;
            }
        }
    } while (TRUE);
}
// ������ͻ������ݺ�Ĵ���������Ҫ����Ԥ���͵����ݴ����ڴ�
static void handle(SOCKETINFO *psocketinfo)
{
    stream *s = newStream();
    char path[256];
    FILE *fp;
    config *con = getConfig();
    http header = psocketinfo->header;
    SOCKET sockClient = psocketinfo->sock;
    int i, total = 0;
    char sendBuf[2048]; //�������ͻ��˵��ַ���
    char *header_buf = NULL;
    char *mime;
    strcpy(path, con->rootpath);
    if ((fp = fopen(strcat(path, header.path), "rb")) || (fp = tryIndex(con->index, path)))
    {//200 OK
        char *ext;
        header_buf = strcpy(psocketinfo->recv_header_buf.buf, "HTTP/1.1 200 ok\r\nContent-type: %s\r\nContent-Length: %d\r\n");
        ext = strrchr(path, '.');
        if (ext != NULL)
        {
            struct stat s;
            stat(path, &s);
            ext++;
            // �ж��Ƿ���Ҫgzipѹ��
            if (con->gzip && s.st_size > con->gzip_min_length && strInArr(con->gzip_file, ext))
            {
                char gz_path[256];
                char hash_str[17];
                int Modified = s.st_mtime;
                strcpy(gz_path, con->gzip_path);
                hashPath(path, hash_str);
                hash_str[16] = 0;
                strcat(strcat(gz_path, hash_str), ".gz");
                fclose(fp);
                // ����gzip���棬���򴴽�
                if ((fp = fopen(gz_path, "rb")) == NULL || Modified > (stat(gz_path, &s), s.st_mtime))
                {
                    file2gzip(path, gz_path, 9);
                    fp = fopen(gz_path, "rb");
                }
                // ���gzip����httpͷ
                strcat(header_buf, "Content-Encoding: gzip\r\n");
            }
        }
        mime = getmime(ext, con->mime);
        if (mime == NULL)
        {
            mime = "text/html; charset=UTF-8";
        }
    }
    else
    {//404 NOT FOUND
        header_buf = strcpy(psocketinfo->recv_header_buf.buf, "HTTP/1.1 404 NOTFOUND\r\nContent-type: %s\r\nContent-Length: %d\r\n");
        mime = "text/html; charset=UTF-8";
        fp = fopen(strcat(strcpy(path, con->rootpath), con->page_404), "rb");
    }
    do
    {
        i = fread(sendBuf, sizeof(BYTE), sizeof(sendBuf), fp);
        total += i;
        stream_push(s, sendBuf, i);
        if (total > MAX_BODY_LENGTH)
            break;
    } while (i == sizeof(sendBuf));
    fclose(fp);
    stringArray *parr = con->header;
    while (parr)
    {
        strcat(header_buf, parr->value);
        parr = parr->next;
    }
    i = sprintf(sendBuf, header_buf, mime, total);
    strcat(sendBuf, "\r\n");
    s = stream_unshift(s, sendBuf, i + 2);
    stream_clear(psocketinfo->recv_body.stream);
    psocketinfo->recv_body.stream = NULL;

    psocketinfo->send_buf.stream = s;
    header_clear(psocketinfo->header.header);
    psocketinfo->header.header = NULL;
    psocketinfo->recv_header_buf.flag = 0;
    psocketinfo->recv_header_buf.length = 0;
    psocketinfo->send_buf.from = 0;
    psocketinfo->state = STATE_SEND;
}
