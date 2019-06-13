#include <winsock2.h>
#include "../until/stream.h"

static inline int sendall(SOCKET sockfd, const char *buf, int len, int *err);
static inline int sendstream(SOCKET sockfd, stream *s, int from, int *err);
static inline void socket_send(SOCKETINFO *psocketinfo);
static inline int sendall(SOCKET sockfd, const char *buf, int len, int *err)
{
    int total = 0, send_len;
    while (len)
    {
        send_len = send(sockfd, buf, len, 0);
        if (send_len == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                return total;
            }
            *err = SOCKET_ERROR;
            return total;
        }
        len -= send_len;
        buf += send_len;
        total += send_len;
    }
    return total;
}
static inline int sendstream(SOCKET sockfd, stream *s, int from, int *err)
{
    int total = 0, send_len;
    while (s)
    {
        if (s->length < from)
        {
            from -= s->length;
            s = s->next;
            continue;
        }
        send_len = sendall(sockfd, s->byte + from, s->length - from, err);
        from = 0;
        if (*err == SOCKET_ERROR)
        {
            return total;
        }
        else
        {
            total += send_len;
            s = s->next;
        }
    }
    return total;
}
static inline void socket_send(SOCKETINFO *psocketinfo)
{
    if (psocketinfo->state != STATE_SEND && psocketinfo->error)
        return;
    int err = 0;
    int send_len;
    send_len = sendstream(psocketinfo->sock, psocketinfo->send_buf.stream, psocketinfo->send_buf.from, &err);
    if (err == SOCKET_ERROR)
    {
        psocketinfo->error = ERR_NET;
        return;
    }
    psocketinfo->send_buf.from += send_len;
}
