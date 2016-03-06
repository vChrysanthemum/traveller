#ifndef __CORE_UTIL_H
#define __CORE_UTIL_H

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <locale.h>

#include "core/frozen.h"

typedef struct Log {
    char *dir;
    FILE *f;
    int fd;
} Log;

/* Anti-warning macro... */
#define NOTUSED(V) ((void) V)

#define AssertOrReturnError(condition, error) if (!condition) return error; 

#define C_LOG_ERROR 1
#define C_LOG_WARNING 2
#define C_LOG_NOTICE 3
#define C_LOG_INFO 4
#define C_LOG_DEBUG 6

#define C_UtilLog(level, FMT, ...) do {\
    char timestr[64];\
    struct timeval tv;\
    gettimeofday(&tv,NULL);\
    strftime(timestr,sizeof(timestr),"%d %b %H:%M:%S.",localtime(&tv.tv_sec));\
    fputc('[', c_log.f); fputs(timestr, c_log.f); fputs("]", c_log.f); \
    fprintf(c_log.f, "(%s:%d) ", __FILE__, __LINE__); fprintf(c_log.f, FMT, ##__VA_ARGS__); \
    fputc('\n', c_log.f); \
    fflush(c_log.f);\
} while(0);

#define C_UtilLogD(FMT, ...) C_UtilLog(C_LOG_DEBUG, FMT, ##__VA_ARGS__)
#define C_UtilLogW(FMT, ...) C_UtilLog(C_LOG_WARNING, FMT, ##__VA_ARGS__)
#define C_UtilLogE(FMT, ...) C_UtilLog(C_LOG_ERROR, FMT, ##__VA_ARGS__)
#define C_UtilLogN(FMT, ...) C_UtilLog(C_LOG_NOTICE, FMT, ##__VA_ARGS__)
#define C_UtilLogI(FMT, ...) C_UtilLog(C_LOG_INFO, FMT, ##__VA_ARGS__)

#define TrvAssert(condition, FMT, ...) do {\
    if (!(condition)) {\
        C_UtilLogI(FMT, ##__VA_ARGS__);\
        exit(-1);\
    }\
} while(0);


#define run_with_period(_ms_) if ((_ms_ <= 1000/server.hz) || !(server.cronloops%((_ms_)/(1000/server.hz))))

int utf8StrWidth (char *str);

#define ALLOW_PATH_SIZE 256

#define C_UtilExit(ERRNO, FMT, ...) do { \
    C_UtilLog(C_LOG_ERROR, FMT, ##__VA_ARGS__); \
    exit(ERRNO); \
} while(0);

sds fileGetContent(char *path);

#define jsonTokToNumber(result, tok, tmpchar) do { \
    memcpy(tmpchar, tok->ptr, tok->len);\
    tmpchar[tok->len] = 0;\
    result = atoi(tmpchar);\
} while(0);

unsigned int dictStringHash(const void *key);
int dictStringCompare(void *privdata, const void *key1,
        const void *key2);
void dictStringDestructor(void *privdata, void *val);
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

sds fileGetContent(char* path);
void escapeQuoteContent(sds result, char **content);

typedef struct doubleString_s {
    char *v1;
    char *v2;
} doubleString_t;
doubleString_t *newDoubleString();
void freeDoubleString(void *_data);

#endif
