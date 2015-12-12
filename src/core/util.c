#include "core/util.h"
#include "core/dict.h"
#include "core/sds.h"

#include <execinfo.h>

dictType stackStringTableDictType = {
    dictStringCaseHash,        /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictStringCompare,         /* key compare */
    NULL,                      /* key destructor */
    NULL                       /* val destructor */
};


int dictSdsKeyCaseCompare(void *privdata, const void *key1,
        const void *key2) {
    NOTUSED(privdata);
    return strcasecmp(key1, key2) == 0;
}

void dictSdsDestructor(void *privdata, void *val) {
    NOTUSED(privdata);
    sdsfree(val);
}

unsigned int dictSdsCaseHash(const void *key) {
    return dictGenCaseHashFunction((unsigned char*)key, sdslen((char*)key));
}

unsigned int dictStringCaseHash(const void *key) {
    return dictGenCaseHashFunction((unsigned char*)key, strlen((char*)key));
}

int dictStringCompare(void *privdata, const void *key1,
        const void *key2) {
    NOTUSED(privdata);
    return strcmp(key1, key2) == 0;
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

void dump(void)
{
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
        TrvLogI("[%02d] %s", j, strings[j]);
    free(strings);
}
