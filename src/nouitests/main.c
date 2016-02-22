#include "core/util.h"
#include "core/adlist.h"

etDevice_t *g_mainDevice;
etDevice_t *g_fooDevice;
etDevice_t *g_netDevice;

char g_basedir[ALLOW_PATH_SIZE] = {""}; /* 绝对路径为 $(traveller)/src */
Log g_log;
Ini *g_conf;

list *g_scripts;
