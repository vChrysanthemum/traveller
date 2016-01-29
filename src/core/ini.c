#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/util.h"
#include "core/ini.h"
#include "core/zmalloc.h"

static void skipWhitespaces(char **ptr)  {
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

static int IniGetId(Ini *conf, char *section, char *option) {
    int j;
    IniOption* opt;
    for (j = 0; j < conf->optionsCount; j++) {
        opt = conf->options[j];

        if (strlen(section) != opt->sectionLen) {
            continue;
        }

        if (0 == strncmp(opt->section, section, opt->sectionLen)) {
            if (0 == strncmp(opt->key, option, opt->keyLen)) {
                return j;
            }
        }
    }

    return -1;
}


Ini *InitIni() {
    Ini *conf;
    conf = zmalloc(sizeof(Ini));
    memset(conf, 0, sizeof(Ini));
    return conf;
}

void IniRead(Ini *conf, char *path) {
    FILE* fp; 
    long len;
    char *content, *ptr;
    int currentContentId, sectionLen=0, id;
    char* section = NULL;
    char tmpSection[512] = "", tmpOption[512] = "";
    IniOption *opt = NULL;

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
    currentContentId = conf->contentsCount - 1;
    conf->contents[currentContentId] = content;

    ptr = content;

    while (0 != *ptr) {
        opt = (IniOption*)zmalloc(sizeof(IniOption));

        skipCommenting(&ptr);

        if (0 == *ptr) {
            break;
        }

        if ('[' == *ptr) {
            ptr++;//skip [
            section = ptr;
            sectionLen = parseSection(ptr);
            ptr += sectionLen;
            ptr++;//skip ]
            skipCommenting(&ptr);
        }

        opt->section = section;
        opt->sectionLen = sectionLen;

        skipWhitespaces(&ptr);
        opt->key = ptr;
        opt->keyLen = parseOptionKeyAndSkip(&ptr);

        skipWhitespaces(&ptr);
        opt->value = ptr;
        opt->valueLen = parseOptionValueAndSkip(&ptr);

        strncpy(tmpSection, section, sectionLen);
        tmpSection[sectionLen] = '\0';
        strncpy(tmpOption, opt->key, opt->keyLen);
        tmpOption[opt->keyLen] = '\0';

        id = IniGetId(conf, tmpSection, tmpOption);
        if (id < 0) {
            conf->optionsCount += 1;
            conf->options = (IniOption**)realloc(conf->options, sizeof(IniOption**) * conf->optionsCount);
            conf->options[conf->optionsCount-1] = opt;
        } else {
            free(conf->options[id]);
            conf->options[id] = opt;
        }

    }
}

IniOption* IniGet(Ini *conf, char *section, char *option) {
    int j = IniGetId(conf, section, option);
    if (-1 == j) {
        return NULL;
    }
    return conf->options[j];
}

void ReleaseIni(Ini **conf) {
    int j;
    for (j = 0; j < (*conf)->contentsCount; j++) {
        free((*conf)->options[j]);
        free((*conf)->contents[j]);
    }
    free(*conf);
    *conf = NULL;
}
