#include "core/util.h"
#include "core/adlist.h"

ETDevice *g_mainDevice;
ETDevice *g_fooDevice;
ETDevice *g_netDevice;

char g_basedir[ALLOW_PATH_SIZE] = {""}; /* 绝对路径为 $(traveller)/src */
Log g_log;
Ini *g_conf;

list *g_scripts;
