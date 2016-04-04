#ifndef __CORE_INI_H
#define __CORE_INI_H

#include <sys/types.h>

#define HAVE_BACKTRACE 1

typedef struct IniOption {
    sds Key;
    sds Value;
} IniOption;

typedef struct IniSection {
    sds  Key;
    dict *Options;
} IniSection;

typedef struct Ini {
    sds *Contents;
    int ContentsCount;
    dict *Sections;
} Ini;

Ini *InitIni();
void IniRead(Ini *conf, char *path);
sds IniGet(Ini *conf, char *sectionKey, char *optionKey);
void FreeIni(Ini *conf);

#define confOptToStr(confOpt, result) do {\
    memcpy(result, confOpt->Value, confOpt->ValueLen);\
    result[confOpt->ValueLen] = 0;\
} while(0);

#define confOptToInt(confOpt, tmpstr, result) do {\
    memcpy(tmpstr, confOpt->Value, confOpt->ValueLen);\
    tmpstr[confOpt->ValueLen] = 0;\
    result = atoi(tmpstr);\
} while(0);

#endif
