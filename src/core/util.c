#include "core/util.h"
#include "core/dict.h"
#include "core/sds.h"

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
