#include "qiof_log.h"
void qiof_err(FILE *fd, const char *header, const char *file, const char *func, int pos, const char *fmt, ...)
{
    time_t clock1;
    struct tm *tptr;
    va_list ap;
    clock1 = time(0);
    tptr = localtime(&clock1);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    fprintf(fd, "%s\033[0;34m[%d.%d.%d,%d:%d:%d,%llu]%s:%d,%s:\033[0m ", header,
            tptr->tm_year + 1900, tptr->tm_mon + 1,
            tptr->tm_mday, tptr->tm_hour, tptr->tm_min,
            tptr->tm_sec, (long long)((tv.tv_usec) / 1000) % 1000, file, pos, func);
    va_start(ap, fmt);
    vfprintf(fd, fmt, ap);
    fprintf(fd, "\n");
    va_end(ap);
}
void Base_Common_Log(IN unsigned long ulFileLine, IN char *pcFileName, IN char *pcFormat, ...)
{
    time_t tmSeconds = time(0);
    struct tm stNowTime;
    va_list stArgs;
    localtime_r(&tmSeconds, &stNowTime);
    printf("[%d-%d-%d %d:%d:%d]", stNowTime.tm_year + 1900, stNowTime.tm_mon + 1,
           stNowTime.tm_mday, stNowTime.tm_hour, stNowTime.tm_min, stNowTime.tm_sec);
    printf("[%s-%lu]", pcFileName, ulFileLine);
    va_start(stArgs, pcFormat);
    vprintf(pcFormat, stArgs);
    va_end(stArgs);
    return;
}
