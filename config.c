
#include "config.h"
#include "until/json.h"
#include "until/object.h"
#include <stdio.h>
#include <sys/stat.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

static config con = {0};

extern config *getConfig()
{
    object w;
    FILE *fp;
    if (*con.rootpath)
    {
        return &con;
    }
    fp = fopen("config.json", "r");
    if (fp)
    {
        struct stat st;
        char *str, buf[2048];
        int len;
        object *window;
        object *temp;
        stat("config.json", &st);
        str = malloc(sizeof(unsigned char) * st.st_size);
        str[0] = 0;
        do
        {
            len = fread(buf, sizeof(char), sizeof(buf), fp);
            strncat(str, buf, len);
        } while (len == sizeof(buf));
        fclose(fp);
        window = json(str);
        temp = getObjectValue(window,"['rootpath']");
        assert(temp);
        strcpy(con.rootpath, temp->value);
        temp = getObjectValue(window,"['index']");
        assert(temp);
        strcpy(con.index, temp->value);
        temp = getObjectValue(window, "['ip']");
        assert(temp);
        strcpy(con.ip, temp->value);
        temp = getObjectValue(window, "['port']");
        assert(temp);
        con.port = temp->i;
        objectfree(window->child);
        free(window);
        return (&con);
    }
}