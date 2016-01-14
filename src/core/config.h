/*
 * ConfigParser https://github.com/vChrysanthemum/ConfigParser
 */
#ifndef _CORE_CONFIG_H
#define _CORE_CONFIG_H
#include <sys/types.h>

#define HAVE_BACKTRACE 1

typedef struct configOption {
    char *section;
    int sectionLen;
    char *key;
    int keyLen;
    char *value;
    int valueLen;
} configOption;

typedef struct config {
    char **contents;
    int contentsCount;
    struct configOption **options;
    int optionsCount;
} config;

config *initConfig();
void configRead(config *conf, char *path);
configOption *configGet(config *conf, char *section, char *option);
void releaseConfig(config **conf);

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
