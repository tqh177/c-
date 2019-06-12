#ifndef LOG_H
#define LOG_H

#ifdef _DEBUG
#define log(format, ...) _logLine(__LINE__,format,##__VA_ARGS__)
// #define log(msg) _logLine(__LINE__,msg)
#else
#define log(format, ...)
#endif
void _logLine(int line, char *format, ...);

#endif