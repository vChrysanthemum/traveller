#ifndef __CORE_UTIL_H
#define __CORE_UTIL_H

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <locale.h>
#include <pthread.h>

#include "core/sds.h"
#include "core/frozen.h"

/* Anti-warning macro... */
#define NOTUSED(V) ((void) V)

#define ERRNO_OK    0
#define ERRNO_ERR   -500
#define ERRNO_NULL  -404

extern FILE* g_logF;
extern pthread_mutex_t g_logMutex;

#define ZeusLog_ERROR 1
#define ZeusLog_WARNING 2
#define ZeusLog_NOTICE 3
#define ZeusLog_INFO 4
#define ZeusLog_DEBUG 6

#define ZeusLog(level, FMT, ...) do {\
    pthread_mutex_lock(&g_logMutex);\
    char timestr[64];\
    struct timeval tv;\
    gettimeofday(&tv,NULL);\
    strftime(timestr,sizeof(timestr),"%d %b %H:%M:%S.",localtime(&tv.tv_sec));\
    fputc('[', g_logF); fputs(timestr, g_logF); fputs("]", g_logF); \
    fprintf(g_logF, "(%s:%d) ", __FILE__, __LINE__); fprintf(g_logF, FMT, ##__VA_ARGS__); \
    fputc('\n', g_logF); \
    fflush(g_logF);\
    pthread_mutex_unlock(&g_logMutex);\
} while(0);

#define ZeusLogD(FMT, ...) ZeusLog(ZeusLog_DEBUG, FMT, ##__VA_ARGS__)
#define ZeusLogW(FMT, ...) ZeusLog(ZeusLog_WARNING, FMT, ##__VA_ARGS__)
#define ZeusLogE(FMT, ...) ZeusLog(ZeusLog_ERROR, FMT, ##__VA_ARGS__)
#define ZeusLogN(FMT, ...) ZeusLog(ZeusLog_NOTICE, FMT, ##__VA_ARGS__)
#define ZeusLogI(FMT, ...) ZeusLog(ZeusLog_INFO, FMT, ##__VA_ARGS__)

#ifdef IS_DEBUG
#include <assert.h>
#define ZeusAssert(condition) assert(condition);
#else
#define ZeusAssert(condition) {}
#endif


#define run_with_period(_ms_) if ((_ms_ <= 1000/server.hz) || !(server.cronloops%((_ms_)/(1000/server.hz))))


int dictSdsKeyCaseCompare(void *privdata, const void *key1,
        const void *key2);
void dictSdsDestructor(void *privdata, void *val);
unsigned int dictSdsCaseHash(const void *key);
unsigned int dictStringCaseHash(const void *key);
int dictStringCompare(void *privdata, const void *key1,
        const void *key2);

#define ALLOW_PATH_SIZE 256

#define ZeusExit(ERRNO, FMT, ...) do { \
    ZeusLog(ZeusLog_ERROR, FMT, ##__VA_ARGS__); \
    exit(ERRNO); \
} while(0);

sds fileGetContent(char *path);

#define jsonTokToNumber(result, tok, tmpchar) do { \
    memcpy(tmpchar, tok->ptr, tok->len);\
    tmpchar[tok->len] = 0x00;\
    result = atoi(tmpchar);\
} while(0);

#endif
