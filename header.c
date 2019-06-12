#include "header.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>

typedef int BOOL;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
// 解析http协议
static char *parseHTTP(http *_http, char *header)
{
    if (memcmp("GET", header, 3) == 0)
    {
        header += 3;
        _http->method = METHOD_GET;
    }
    else if (memcmp("POST", header, 4) == 0)
    {
        header += 4;
        _http->method = METHOD_POST;
    }
    else if (memcmp("HEAD", header, 4) == 0)
    {
        header += 4;
        _http->method = METHOD_HEAD;
    }
    else if (memcmp("OPTION", header, 6) == 0)
    {
        header += 6;
        _http->method = METHOD_OPTION;
    }

    if (memcmp(" /", header++, 2) == 0)
    {
        int i;
        for (i = 0; *header != '?' && *header != ' ' && i < sizeof(_http->path); i++)
        {
            _http->path[i] = *header;
            header++;
        }
        _http->path[i] = 0;
        if (*header++ == '?')
        {
            char *p = memccpy(_http->query, header, ' ', sizeof(_http->query) - 1);
            if (p)
            {
                *(p - 1) = 0;
                header += p - _http->query;
            }
			else
			{
				return NULL;
			}
        }
        else
        {
            _http->query[0] = 0;
        }
        if (memcmp("HTTP/", header, 5) == 0)
        {
            _http->protocol = PROTOCOL_HTTP;
            header += 5;
            _http->version = ((*header - '0') << 8) | ((*(header + 2) - '0'));
            return header + 5;
        }
        return NULL;
    }
    return NULL;
}
// 解析header字典
static char *parseHeader(http *_http, char *header)
{
    dict h = {0}, *dheader = &h;
    char key[1024];
    char value[1024];
    char *p;
    while (header)
    {
        p = memccpy(key, header, ':', sizeof(key));
        *(p - 1) = 0;
        header += p - key;
        header++;
        p = memccpy(value, header, '\r', sizeof(value));
        *(p - 1) = 0;
        header += p - value;
        header++;
        if (_http)
        {
            dheader->next = newHeader();
            dheader = dheader->next;
            strcpy(dheader->key, key);
            strcpy(dheader->value, value);
        }
        if (memcmp("\r\n", header, 2) == 0)
        {
            break;
        }
    }
    if (header&&_http)
    {
        _http->header = h.next;
    }
    return header;
}

dict *newHeader()
{
    dict *h = malloc(sizeof(dict));
    memset(h, 0, sizeof(dict));
    return h;
}

void header_clear(dict *header)
{
    if (header)
    {
        header_clear(header->next);
        free(header);
    }
}

// 解析header
http parse_header(char *header)
{
    http h = {0};
    header = parseHTTP(&h, header);
    header = parseHeader(&h, header);
    return h;
}

// 获取header键值对
char *getHeader(dict *header, const char *key)
{
    while (header)
    {
        if (strcmp(header->key, key) == 0)
        {
            return header->value;
        }
        header = header->next;
    }
    return NULL;
}