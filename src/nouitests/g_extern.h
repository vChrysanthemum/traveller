#ifndef __G_EXTERN_H
#define __G_EXTERN_H

#include "core/ini.h"
#include "event/event.h"
#include "net/networking.h"
#include "core/util.h"

#include "lua.h"
#include "sqlite3.h"

extern ETDevice *g_mainDevice;
extern ETDevice *g_fooDevice;
extern ETDevice *g_netDevice;

extern char g_basedir[];
Log g_log;
extern Ini *g_conf;

list *g_scripts;

#endif
