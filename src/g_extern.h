#ifndef __G_EXTERN_H
#define __G_EXTERN_H

#include "core/config.h"
#include "event/event.h"
#include "net/networking.h"

#include "lua.h"
#include "sqlite3.h"

extern ETDevice *g_mainDevice;
extern ETDevice *g_fooDevice;
extern ETDevice *g_netDevice;

extern char g_basedir[ALLOW_PATH_SIZE]; /* 绝对路径为 $(traveller)/src */
extern char *g_logdir;
extern FILE* g_logF;
extern int g_logFInt;
extern config *g_conf;

/* UI部分 */

/* 服务端模式所需变量 */
extern char g_srvGalaxydir[ALLOW_PATH_SIZE]; /* 需要加载的星系路径 */
extern lua_State *g_srvLuaSt;
extern sqlite3 *g_srvDB;

/* 客户端模式所需变量 */
extern char g_cliGalaxydir[ALLOW_PATH_SIZE]; /* 需要加载的星系路径 */
extern lua_State *g_cliLuaSt;
extern sqlite3 *g_cliDB;
extern NTSnode *g_galaxiesSrvSnode; /* 星系服务端连接 */

#endif
