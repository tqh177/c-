#ifndef HEADER_H
#define HEADER_H

#define MAX_HEADER_LENGTH 2048
#define PROTOCOL_HTTP 1
#define PROTOCOL_HTTPS 2
#define PROTOCOL_TCP 3
#define PROTOCOL_UDP 4
#define PROTOCOL_FTP 5

#define METHOD_GET 1
#define METHOD_POST 2
#define METHOD_HEAD 3
#define METHOD_OPTION 4

typedef struct dict
{
    struct dict *next;
    char key[255];
    char value[1024];
} dict;

typedef struct http
{
    dict *header;//(◊¢“‚«Â¿Ì)
    short method;
    short version;
    short protocol;
    char path[256];
    char query[1024];
} http;

dict* newHeader();
void header_clear(dict *header);
http parse_header(char *header);
char *getHeader(dict *header, const char *key);

#endif