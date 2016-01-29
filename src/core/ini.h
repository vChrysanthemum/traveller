#ifndef _CORE_INI_H
#define _CORE_INI_H
#include <sys/types.h>

#define HAVE_BACKTRACE 1

typedef struct IniOption {
    char *section;
    int sectionLen;
    char *key;
    int keyLen;
    char *value;
    int valueLen;
} IniOption;

typedef struct Ini {
    char **contents;
    int contentsCount;
    struct IniOption **options;
    int optionsCount;
} Ini;

Ini *InitIni();
void IniRead(Ini *conf, char *path);
IniOption *IniGet(Ini *conf, char *section, char *option);
void ReleaseIni(Ini **conf);

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
