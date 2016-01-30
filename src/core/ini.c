#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/util.h"
#include "core/ini.h"
#include "core/dict.h"
#include "core/zmalloc.h"

static IniOption* NewOption() {
    IniOption *option = (IniOption*)zmalloc(sizeof(IniOption));
    option->key = sdsempty();
    option->value = sdsempty();
    return option;
}

static void dictFreeOption(void *privdata, void *_option) {
    DICT_NOTUSED(privdata);
    IniOption *option = (IniOption*)_option;

    sdsfree(option->key);
    sdsfree(option->value);
    zfree(option);
}

static dictType sectionOptionDictType = {
    dictStringHash,            /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictStringCompare,         /* key compare */
    dictStringDestructor,      /* key destructor */
    dictFreeOption,            /* val destructor */
};

static void dictFreeSection(void *privdata, void* _section) {
    DICT_NOTUSED(privdata);
    IniSection *section = (IniSection*)_section;

    sdsfree(section->key);
    dictRelease(section->options);
    zfree(section);
}

dictType sectionDictType = {
    dictStringHash,            /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictStringCompare,         /* key compare */
    dictStringDestructor,      /* key destructor */
    dictFreeSection,           /* val destructor */
};

static IniSection* NewSection() {
    IniSection *section = (IniSection*)zmalloc(sizeof(IniSection));
    section->key = sdsempty();
    section->options = dictCreate(&sectionOptionDictType, NULL);
    return section;
}

static inline void skipWhitespaces(char **ptr)  {
    while( 0 != **ptr && (' ' == **ptr || '\t' == **ptr || '\r' == **ptr || '\n' == **ptr)) {
        (*ptr)++;
    }
}

static void skipCommenting(char **ptr)  {
    int ifFound;

    while (1) {
        ifFound = 0;

        skipWhitespaces(ptr);

        if ('#' == **ptr) {
            ifFound = 1;
            while(0 != **ptr && '\r' != **ptr && '\n' != **ptr) {
                (*ptr)++;
            }

            skipWhitespaces(ptr);
        }

        if (0 == ifFound) break;

    }
}

static int parseSection(char *ptr) {
    int n = 0;
    while (']' != *ptr && 0 != *ptr) {
        n++;
        ptr++;
    }
    return n;
}

static int parseOptionKeyAndSkip(char **ptr) {
    int n=0, isEqualFound=0;
    while (' ' != **ptr && '\t' != **ptr && 0 != **ptr) {
        if ('=' == **ptr) {
            isEqualFound = 1;
            break;
        }

        n++;
        (*ptr)++;
    }

    skipWhitespaces(ptr);

    if(1 == isEqualFound) {
        (*ptr)++;

    } else {
        while ('=' != **ptr && 0 != **ptr) {
            (*ptr)++;
        }
        (*ptr)++;
    }

    return n;
}

static int parseOptionValueAndSkip(char **ptr) {
    int n=0, whitespaceCount=0;
    char *_ptr;
    while (1) {
        whitespaceCount = 0;
        _ptr = *ptr;
        while(' ' == *_ptr || '\t' == *_ptr) {
            whitespaceCount += 1;
            _ptr = (*ptr) + whitespaceCount;
        }

        if ('\n' == **ptr) {
            (*ptr)++;
            return n;
        }

        (*ptr) += whitespaceCount;
        n += whitespaceCount;

        (*ptr)++;
        n++;
    }

    return n;
}

static inline IniSection* MustGetSection(Ini *conf, char *key) {
    IniSection *section = (IniSection*)dictFetchValue(conf->sections, key);

    if (0 == section) {
        section = NewSection();
        section->key = sdscat(section->key, key);
        dictAdd(conf->sections, stringnew(key), section);
    }

    return section;
}


Ini *InitIni() {
    Ini *conf;
    conf = (Ini*)zmalloc(sizeof(Ini));
    memset(conf, 0, sizeof(Ini));
    conf->sections = dictCreate(&sectionDictType, 0);
    return conf;
}

void IniRead(Ini *conf, char *path) {
    FILE* fp; 
    long len;
    char *content, *ptr;
    sds sectionKey = sdsempty();
    char *optionKey = 0, *optionValue = 0;
    int sectionKeyLen = 0, optionKeyLen = 0, optionValueLen = 0;
    IniOption *option = 0;
    IniSection *section = 0;

    fp = fopen(path, "r");
    if (fp == NULL) {
        return;
    }   

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (len <= 0) {
        return;
    }

    content = zmalloc(len + 1);
    memset(content, 0, len + 1);
    fread(content, len, 1, fp);

    conf->contentsCount += 1;
    conf->contents = (char **)realloc(conf->contents, sizeof(char **) * conf->contentsCount);
    conf->contents[conf->contentsCount-1] = content;

    ptr = content;

    while (0 != *ptr) {
        skipCommenting(&ptr);

        if (0 == *ptr) {
            break;
        }

        if ('[' == *ptr) {
            ptr++;//skip [

            sectionKeyLen = parseSection(ptr);
            sectionKey = sdscpylen(sectionKey, ptr, sectionKeyLen);
            section = MustGetSection(conf, sectionKey);

            ptr += sectionKeyLen;
            ptr++;//skip ]
            skipCommenting(&ptr);

        }

        skipWhitespaces(&ptr);
        optionKey = ptr;
        optionKeyLen= parseOptionKeyAndSkip(&ptr);

        skipWhitespaces(&ptr);
        optionValue = ptr;
        optionValueLen = parseOptionValueAndSkip(&ptr);

        if (0 != section) {
            option = NewOption();
            option->key = sdscpylen(option->key, optionKey, optionKeyLen);
            option->value = sdscpylen(option->value, optionValue, optionValueLen);
            dictReplace(section->options, stringnewlen(optionKey, optionKeyLen), option);
        }
    }

    sdsfree(sectionKey);
}

sds IniGet(Ini *conf, char *sectionKey, char *optionKey) {
    IniSection *section = (IniSection*)dictFetchValue(conf->sections, sectionKey);
    if (0 == section) {
        return 0;
    }

    IniOption *option = (IniOption*)dictFetchValue(section->options, optionKey);
    if (0 == option) {
        return 0;
    }

    return option->value;
}

void FreeIni(Ini *conf) {
    int j;
    for (j = 0; j < conf->contentsCount; j++) {
        free(conf->contents[j]);
    }
    dictRelease(conf->sections);
    free(conf);
}
