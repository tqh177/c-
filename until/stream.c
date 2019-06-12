#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "stream.h"

static inline stream *_stream_end(stream *s);
static inline size_t _stream_fill(stream *s, BYTE bytes[], size_t bytes_len);

stream *_stream_end(stream *s)
{
    while (s->next)
    {
        s = s->next;
    }
    return s;
}

size_t _stream_fill(stream *s, BYTE bytes[], size_t bytes_len)
{
    if (s->length == STREAM_MAX_LEN)
    {
        return 0;
    }
    else
    {
        size_t len = STREAM_MAX_LEN - s->length;
        if (bytes_len < len)
        {
            len = bytes_len;
        }
        memcpy(s->byte + s->length, bytes, len);
        s->length += len;
        return len;
    }
}

inline stream *newStream()
{
    stream *s = malloc(sizeof(stream));
    s->length = 0;
    s->next = NULL;
    // memset(s, 0, sizeof(stream));
    return s;
}

void stream_push(stream *s, BYTE bytes[], size_t bytes_len)
{
    size_t len;
    s = _stream_end(s);
    // len = _stream_fill(s, bytes, bytes_len);
    // while (bytes_len -= len)
    // {
    //     bytes += len;
    //     s->next = newStream();
    //     s = s->next;
    //     len = _stream_fill(s, bytes, bytes_len);
    // }
    do
    {
        len = _stream_fill(s, bytes, bytes_len);
        bytes_len -= len;
        if (bytes_len <= 0)
            break;
        bytes += len;
        s->next = newStream();
        s = s->next;
    } while (1);
}
stream *stream_unshift(stream *s, BYTE bytes[], size_t bytes_len)
{
    stream *s1 = newStream();
    stream_push(s1, bytes, bytes_len);
    stream_connect(s1, s);
    return s1;
}
void stream_clear(stream *s)
{
    if (s)
    {
        stream_clear(s->next);
        free(s);
    }
}
size_t stream_len(stream *s)
{
    size_t len = s->length;
    while (s->next)
    {
        s = s->next;
        len += s->length;
    }
    return len;
}
void stream_connect(stream *s, stream *s1)
{
    s = _stream_end(s);
    s->next = s1;
}
char *stream2string(stream *s)
{
    size_t len = stream_len(s);
    char *str = malloc(sizeof(char) * (len + 1));
    char *hstr = str;
    str[len] = 0;
    do
    {
        memcpy(str, s->byte, s->length);
        str += s->length;
        s = s->next;
    } while (s);
    return hstr;
}
