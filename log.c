#include "log.h"
#include <stdio.h>
#include <stdarg.h>

void _logLine(int line,char *format, ...)
{
    char buffer[2048];
    va_list aptr;
    va_start(aptr, format);
    vsprintf(buffer, format, aptr);
    printf("line[%d]:%s\n", line, buffer);
    va_end(aptr);
}