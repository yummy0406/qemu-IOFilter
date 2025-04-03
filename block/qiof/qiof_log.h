#ifndef QIOF_LOG_H
#define QIOF_LOG_H
#include<time.h>
#include<stdio.h>
#include<sys/time.h>
#include<stdarg.h>
#define     IN 
#define     OUT 
#define     INOUT
#define QIOF_LOG(format, ...) fprintf(stdout, format, ##__VA_ARGS__)
//#define QIOF_LOG(format, args...) Base_Common_Log(__LINE__, __FILE__, format, ##args)
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define QIOF_ERR(...) qiof_err(stderr, "\033[1;31m[ERROR]\033[0m", __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
void qiof_err(FILE *fd, const char *header, const char *file, const char *func, int pos, const char *fmt, ...);
void Base_Common_Log(IN unsigned long ulFileLine, IN char *pcFileName, IN char *pcFormat, ...);
#endif