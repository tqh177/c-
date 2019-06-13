#ifndef CONFIG_H
#define CONFIG_H
typedef struct pageindex
{
    char *page;
    struct pageindex *next;
} pageindex;

typedef struct mime
{
    char *key;
    char *value;
    struct mime *next;
} mime;

typedef struct config
{
    char rootpath[256];
    char host[126];
    pageindex *index;
    char ip[16];
    unsigned short port;
    mime *mime;
} config;

config *getConfig();
#endif