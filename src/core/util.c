#include <execinfo.h>
#include <string.h>

#include "core/zmalloc.h"
#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/util.h"

Log  c_log;

dictType stringTableDictType = {
    dictStringCaseHash,        /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictStringCompare,         /* key compare */
    dictStringDestructor,      /* key destructor */
    dictStringDestructor       /* val destructor */
};

dictType stackStringTableDictType = {
    dictStringCaseHash,        /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictStringCompare,         /* key compare */
    NULL,                      /* key destructor */
    NULL                       /* val destructor */
};

dictType sdskvDictType = {
    dictSdsHash,               /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictSdsKeyCompare,         /* key compare */
    dictSdsDestructor,         /* key destructor */
    dictSdsDestructor,         /* val destructor */
};

/*====================== Hash table type implementation  ==================== */

/* This is a hash table type that uses the SDS dynamic strings library as
 *  * keys and radis objects as values (objects can hold SDS strings,
 *   * lists, sets). */

unsigned int dictStringHash(const void *key) {
    return dictGenHashFunction((unsigned char*)key, strlen((char*)key));
}

int dictStringCompare(void *privdata, const void *key1,
        const void *key2)
{
    int l1,l2;
    DICT_NOTUSED(privdata);

    l1 = strlen((char*)key1);
    l2 = strlen((char*)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

void dictStringDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
    zfree(val);
}

unsigned int dictSdsHash(const void *key) {
    return dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
}

unsigned int dictSdsCaseHash(const void *key) {
    return dictGenCaseHashFunction((unsigned char*)key, sdslen((char*)key));
}

void dictVanillaFree(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
    zfree(val);
}

void dictListDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
    listRelease((list*)val);
}

void dictSdsDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
    sdsfree(val);
}

int dictSdsKeyCompare(void *privdata, const void *key1,
        const void *key2)
{
    int l1,l2;
    DICT_NOTUSED(privdata);

    l1 = sdslen((sds)key1);
    l2 = sdslen((sds)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

int dictSdsKeyCaseCompare(void *privdata, const void *key1,
        const void *key2) {
    NOTUSED(privdata);
    return strcasecmp(key1, key2) == 0;
}

unsigned int dictStringCaseHash(const void *key) {
    return dictGenCaseHashFunction((unsigned char*)key, strlen((char*)key));
}

sds fileGetContent(char *path) {
    FILE* fp; 
    long len;
    sds content;

    fp = fopen(path, "r");
    if (fp == NULL) {
        return NULL;
    }   

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (len <= 0) {
        return NULL;
    }
    content = sdsMakeRoomFor(sdsempty(), len);
    fread(content, len, 1, fp);
    sdsupdatelen(content);
    return content;
}

void dump(void) {
    int j, nptrs;
    void *buffer[100];
    char **strings;
    nptrs = backtrace(buffer, 3);
    //printf("backtrace() returned %d addresses\n", nptrs);
    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }   
    for (j = 0; j < nptrs; j++)
        C_UtilLogI("[%02d] %s", j, strings[j]);
    free(strings);
}

//获取 utf8 字符串占用屏幕宽度
int utf8StrWidth (char *str) {
    int result = 0;
    int str_len = strlen(str);
    int utf8num = 0;
    for (int i = 0; i < str_len; i++) {
        if (str[i] < 0) {
            utf8num++;
            if (utf8num >= 3) {
                result += 2;
                utf8num = 0;
            }
        } else {
            result++;
        }
    }

    return result;
}

void escapeQuoteContent(sds result, char **content) {
    int isExpectingDoubleQuoteClose = 0;
    int isExpectingQuoteClose = 0;
    if ('\'' == **content) isExpectingQuoteClose = 1;
    else if ('"' == **content) isExpectingDoubleQuoteClose = 1;
    else return;

    int offset = -1;
    while (1) {
        offset++;
        (*content)++;

        if ('\0' == **content) {
            break;
        }

        if ('\\' == **content) {
            (*content)++;
            result[offset] = **content;
            continue;
        }

        if (1 == isExpectingQuoteClose && '\'' == **content) {
            (*content)++;
            break;
        }

        if (1 == isExpectingDoubleQuoteClose && '"' == **content) {
            (*content)++;
            break;
        }

        result[offset] = **content;
    }

    result[offset] = '\0';

    sdsupdatelen(result);
}

doubleString_t *newDoubleString() {
    doubleString_t *result = (doubleString_t*)zmalloc(sizeof(doubleString_t));
    memset(result, 0, sizeof(doubleString_t));
    return result;
}

void freeDoubleString(void *_data) {
    doubleString_t* data = (doubleString_t*)_data;
    zfree(data->v1);
    zfree(data->v2);
    zfree(data);
}
