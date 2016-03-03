#ifndef __G_EXTERN_H
#define __G_EXTERN_H

#include "core/ini.h"
#include "event/event.h"
#include "net/networking.h"
#include "core/util.h"

#include "lua.h"
#include "sqlite3.h"

extern etDevice_t *g_mainDevice;
extern etDevice_t *g_fooDevice;
extern etDevice_t *g_netDevice;

extern char g_basedir[];
Log c_log;
extern Ini *g_conf;

list *g_scripts;

#endif
