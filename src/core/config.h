/*
 * ConfigParser https://github.com/vChrysanthemum/ConfigParser
 */
#ifndef _CORE_CONFIG_H
#define _CORE_CONFIG_H
#include <sys/types.h>

#define HAVE_BACKTRACE 1

struct configOption {
    char *section;
    int sectionLen;
    char *key;
    int keyLen;
    char *value;
    int valueLen;
};

struct config {
    char **contents;
    int contentsCount;
    struct configOption **options;
    int optionsCount;
};

struct config *initConfig();
void configRead(struct config *conf, char *path);
struct configOption *configGet(struct config *conf, char *section, char *option);
void releaseConfig(struct config **conf);

#define confOptToStr(confOpt, result) do {\
    memcpy(result, confOpt->value, confOpt->valueLen);\
    result[confOpt->valueLen] = 0;\
} while(0);

#define confOptToInt(confOpt, tmpstr, result) do {\
    memcpy(tmpstr, confOpt->value, confOpt->valueLen);\
    tmpstr[confOpt->valueLen] = 0;\
    result = atoi(tmpstr);\
} while(0);

#endif
