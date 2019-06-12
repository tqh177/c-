typedef struct config
{
    char rootpath[256];
    char host[126];
    char index[32];
    char ip[16];
    unsigned short port;
}config;

config *getConfig();