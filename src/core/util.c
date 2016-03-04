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

sds file_get_contents(char* path) {
    FILE* fp;
    long len;
    sds ret;

    fp = fopen(path, "r");
    if (0 == fp) {
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (len < 0) {
        ret = 0;
    } else {
        ret = sdsnewlen(0, len + 1);
        fread(ret, len, 1, fp);
    }

    fclose(fp);

    return ret;
}
