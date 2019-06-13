
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
        pageindex index, *pindex = &index;
        mime mime, *pmime = &mime;
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
        temp = getObjectValue(window, "['rootpath']");
        assert(temp);
        strcpy(con.rootpath, temp->value);
        temp = getObjectValue(window, "['index']");
        assert(temp && temp->type == OBJECT_ARRAY);
        temp = temp->child;
        while (temp && temp->type == OBJECT_STRING)
        {
            pindex->next = malloc(sizeof(pageindex));
            pindex->next->page = malloc(sizeof(char) * strlen(temp->value) + 1);
            strcpy(pindex->next->page, temp->value);
            pindex = pindex->next;
            pindex->next = NULL;
            temp = temp->brother;
        }
        con.index = index.next;
        temp = getObjectValue(window, "['mime']");
        assert(temp && temp->type == OBJECT_OBJECT);
        temp = temp->child;
        while (temp)
        {
            pmime->next = malloc(sizeof(mime));
            pmime->next->key = malloc(sizeof(char) * strlen(temp->key) + 1);
            pmime->next->value = malloc(sizeof(char) * strlen(temp->value) + 1);
            strcpy(pmime->next->key, temp->key);
            strcpy(pmime->next->value, temp->value);
            pmime = pmime->next;
            pmime->next = NULL;
            temp = temp->brother;
        }
        con.mime = mime.next;
        temp = getObjectValue(window, "['ip']");
        assert(temp);
        strcpy(con.ip, temp->value);
        temp = getObjectValue(window, "['port']");
        assert(temp);
        con.port = temp->i;
        objectfree(window->child);
        free(window);
        puts("读取配置文件成功");
        return (&con);
    }
    else
    {
        puts("config.json文件不存在");
        exit(-1);
    }
}