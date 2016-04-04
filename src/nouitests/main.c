#include "lua.h"

#include "core/sds.h"
#include "core/dict.h"
#include "core/adlist.h"
#include "core/ini.h"
#include "core/util.h"
#include "core/extern.h"

#include "event/event.h"
#include "net/networking.h"

etDevice_t *g_mainDevice;
etDevice_t *g_fooDevice;
etDevice_t *g_netDevice;

char *g_scriptBaseDir;
char g_basedir[ALLOW_PATH_SIZE] = {""}; /* 绝对路径为 $(traveller)/src */
Ini *g_conf;

list *g_scripts;
