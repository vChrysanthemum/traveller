#ifndef __CORE_UTIL_H
#define __CORE_UTIL_H

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <locale.h>

#include "core/dict.h"
#include "core/adlist.h"
#include "core/zmalloc.h"
#include "core/sds.h"
#include "core/frozen.h"

/* Anti-warning macro... */
#define NOTUSED(V) ((void) V)

extern FILE* g_logF;

#define TrvLog_ERROR 1
#define TrvLog_WARNING 2
#define TrvLog_NOTICE 3
#define TrvLog_INFO 4
#define TrvLog_DEBUG 6

#define TrvLog(level, FMT, ...) do {\
    char timestr[64];\
    struct timeval tv;\
    gettimeofday(&tv,NULL);\
    strftime(timestr,sizeof(timestr),"%d %b %H:%M:%S.",localtime(&tv.tv_sec));\
    fputc('[', g_logF); fputs(timestr, g_logF); fputs("]", g_logF); \
    fprintf(g_logF, "(%s:%d) ", __FILE__, __LINE__); fprintf(g_logF, FMT, ##__VA_ARGS__); \
    fputc('\n', g_logF); \
    fflush(g_logF);\
} while(0);

#define TrvLogD(FMT, ...) TrvLog(TrvLog_DEBUG, FMT, ##__VA_ARGS__)
#define TrvLogW(FMT, ...) TrvLog(TrvLog_WARNING, FMT, ##__VA_ARGS__)
#define TrvLogE(FMT, ...) TrvLog(TrvLog_ERROR, FMT, ##__VA_ARGS__)
#define TrvLogN(FMT, ...) TrvLog(TrvLog_NOTICE, FMT, ##__VA_ARGS__)
#define TrvLogI(FMT, ...) TrvLog(TrvLog_INFO, FMT, ##__VA_ARGS__)

#ifdef IS_DEBUG
#include <assert.h>
#define TrvAssert(condition) assert(condition);
#else
#define TrvAssert(condition) {}
#endif


#define run_with_period(_ms_) if ((_ms_ <= 1000/server.hz) || !(server.cronloops%((_ms_)/(1000/server.hz))))


int utf8StrWidth (char *str);

#define ALLOW_PATH_SIZE 256

#define TrvExit(ERRNO, FMT, ...) do { \
    TrvLog(TrvLog_ERROR, FMT, ##__VA_ARGS__); \
    exit(ERRNO); \
} while(0);

sds fileGetContent(char *path);

#define jsonTokToNumber(result, tok, tmpchar) do { \
    memcpy(tmpchar, tok->ptr, tok->len);\
    tmpchar[tok->len] = 0;\
    result = atoi(tmpchar);\
} while(0);

int dictSdsKeyCaseCompare(void *privdata, const void *key1,
        const void *key2);
unsigned int dictSdsHash(const void *key);
unsigned int dictSdsCaseHash(const void *key);
void dictVanillaFree(void *privdata, void *val);
void dictListDestructor(void *privdata, void *val);
void dictSdsDestructor(void *privdata, void *val);
int dictSdsKeyCompare(void *privdata, const void *key1,
        const void *key2);
int dictStringCompare(void *privdata, const void *key1,
        const void *key2);
unsigned int dictStringCaseHash(const void *key);

#endif
