#include "object.h"
#include "stream.h"
#include <stdlib.h>
#include <string.h>

static void _obj2stream1(object *obj, stream *s);
static void _obj2stream(object *obj, stream *s);

// 创建新的对象
object *newObject()
{
    object *obj = malloc(sizeof(object));
    memset(obj, 0, sizeof(object));
    return obj;
}

// 判断是否有key,有则推入缓冲区
static void _obj2stream1(object *obj, stream *s)
{
    if (obj == NULL)
    {
        return;
    }
    else if (obj->parent->type == OBJECT_OBJECT)
    {
        stream_push(s, "\"", 1);
        stream_push(s, obj->key, strlen(obj->key));
        stream_push(s, "\":", 2);
    }
    _obj2stream(obj, s);
    if (obj->brother)
    {
        stream_push(s, ",", 1);
        _obj2stream1(obj->brother, s);
    }
}
// 根据对象类型转json
static void _obj2stream(object *obj, stream *s)
{
    switch (obj->type)
    {
    case OBJECT_OBJECT:
        stream_push(s, "{", 1);
        _obj2stream1(obj->child, s);
        stream_push(s, "}", 1);
        break;
    case OBJECT_ARRAY:
        stream_push(s, "[", 1);
        _obj2stream1(obj->child, s);
        stream_push(s, "]", 1);
        break;
    case OBJECT_STRING:
        stream_push(s, "\"", 1);
        stream_push(s, obj->value, strlen(obj->value));
        stream_push(s, "\"", 1);
        break;
    case OBJECT_BOOL:
    {
        char *c = obj->i ? "true" : "false";
        stream_push(s, c, strlen(c));
        break;
    }
    case OBJECT_NUMBER:
    {
        char str[20];
        ltoa(obj->i, str, 10);
        stream_push(s, str, strlen(str));
        break;
    }
    case OBJECT_NULL:
        stream_push(s, "null", 4);
        break;
    default:
        break;
    }
}

char *json_encode(object *obj)
{
    stream *s = newStream();
    char *str;
    _obj2stream(obj, s);
    str = stream2string(s);
    stream_clear(s);
    return str;
}

void objectfree(object *obj)
{
    if (obj)
    {
        if (obj->parent->type == OBJECT_OBJECT)
        {
            free(obj->key);
        }
        switch (obj->type)
        {
        case OBJECT_OBJECT:
        case OBJECT_ARRAY:
            objectfree(obj->child);
            break;
        case OBJECT_STRING:
            free(obj->value);
            break;
        default:
            break;
        }
        objectfree(obj->brother);
        free(obj);
    }
}
// 跳过空白符
static char *skipSpace(char *str)
{
    while (*str&&strchr(" \r\n\t", *str))
    {
        str++;
    }
    return str;
}
// 获取字符串
static char *getStr(char *str, char **str1)
{
    int i = 1;
    if (*str == '\'')
    {
        int flag = 0;
        while (str[i] != '\'' || flag)
        {
            if (str[i] == '\\')
            {
                flag = !flag;
            }
            else
            {
                flag = 0;
            }
            i++;
        }
        if (str1)
        {
            *str1 = malloc(sizeof(char) * i);
            strncpy(*str1, str + 1, i - 1);
            (*str1)[i - 1] = 0;
        }
        str = &(str[i + 1]);
    }
    else
    {
        return NULL;
    }
    return str;
}

object *getObjectValue(object *obj, char *s)
{
    if(obj==NULL)
        return NULL;
    s = skipSpace(s);
    if (*s == '[')
    {
        char *key;
        if (s && *(++s) == '\'' && obj->type == OBJECT_OBJECT)
        {
            s = getStr(s, &key);
            s = skipSpace(s);
            if (*s == ']')
            {
                obj = obj->child;
                while (obj)
                {
                    if (strcmp(obj->key, key) == 0)
                    {
                        return getObjectValue(obj, s+1);
                    }
                    else
                    {
                        obj = obj->brother;
                    }
                }
            }
        }
        else if (s && strchr("0123456789", *s) && obj->type == OBJECT_ARRAY)
        {
            int i = 0, flag = 1, j;
            obj = obj->child;
            if (obj == NULL)
                return NULL;
            do
            {
                i *= flag;
                i += *s - '0';
                flag *= 10;
            } while (strchr("0123456789", *s));
            for (j = 0; j < i; j++)
            {
                obj = obj->brother;
            }
            return getObjectValue(obj, s);
        }
    }
    else if (*s == '\0')
    {
        return obj;
    }
    return NULL;
}
