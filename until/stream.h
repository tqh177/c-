#ifndef STREAM_H
#define STREAM_H

typedef unsigned char BYTE;
typedef unsigned int size_t;
#define STREAM_MAX_LEN 2048

typedef struct stream
{
    BYTE byte[STREAM_MAX_LEN];
    size_t length;
    struct stream *next;
} stream;

stream *newStream();
void stream_push(stream *s, BYTE bytes[], size_t bytes_len);
stream *stream_unshift(stream *s, BYTE bytes[], size_t bytes_len);
void stream_clear(stream *s);
char *stream2string(stream *s);
size_t stream_len(stream *s);
void stream_connect(stream *s, stream *s1);

#endif