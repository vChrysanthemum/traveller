/*
 * ConfigParser https://github.com/vChrysanthemum/ConfigParser
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/util.h"
#include "core/config.h"
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

static int configGetReturnId(struct config *conf, char *section, char *option) {
    int j;
    struct configOption* opt;
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


struct config *initConfig() {
    struct config *conf;
    conf = zmalloc(sizeof(struct config));
    memset(conf, 0, sizeof(struct config));
    return conf;
}

void configRead(struct config *conf, char *path) {
    FILE* fp; 
    long len;
    char *content, *ptr;
    int currentContentId, sectionLen=0, id;
    char* section = NULL;
    char tmpSection[512] = "", tmpOption[512] = "";
    struct configOption *opt = NULL;

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
        opt = (struct configOption*)zmalloc(sizeof(struct configOption));

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

        id = configGetReturnId(conf, tmpSection, tmpOption);
        if (id < 0) {
            conf->optionsCount += 1;
            conf->options = (struct configOption**)realloc(conf->options, sizeof(struct configOption**) * conf->optionsCount);
            conf->options[conf->optionsCount-1] = opt;
        } else {
            free(conf->options[id]);
            conf->options[id] = opt;
        }

    }
}

struct configOption* configGet(struct config *conf, char *section, char *option) {
    int j = configGetReturnId(conf, section, option);
    if (-1 == j) {
        return NULL;
    }
    return conf->options[j];
}

void releaseConfig(struct config **conf) {
    int j;
    for (j = 0; j < (*conf)->contentsCount; j++) {
        free((*conf)->options[j]);
        free((*conf)->contents[j]);
    }
    free(*conf);
    *conf = NULL;
}
