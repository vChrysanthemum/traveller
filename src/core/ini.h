#ifndef __CORE_INI_H
#define __CORE_INI_H

#include <sys/types.h>

#include "core/sds.h"
#include "core/dict.h"
#include "core/adlist.h"

#define HAVE_BACKTRACE 1

typedef struct IniOption {
    sds key;
    sds value;
} IniOption;

typedef struct IniSection {
    sds  key;
    dict *options;
} IniSection;

typedef struct Ini {
    sds *contents;
    int contentsCount;
    dict *sections;
} Ini;

Ini *InitIni();
void IniRead(Ini *conf, char *path);
sds IniGet(Ini *conf, char *sectionKey, char *optionKey);
void FreeIni(Ini *conf);

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
