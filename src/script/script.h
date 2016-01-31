#ifndef __SCRIPT_SCRIPT_H
#define __SCRIPT_SCRIPT_H

#include "core/util.h"
#include "net/networking.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "sqlite3.h"

#define LUA_SERVICE_ERRNO_OK         0
#define LUA_SERVICE_ERRNO_INNERERR   502

#define STAssertLuaPCallSuccess(L, errno) do {\
    if (0 != errno) {\
        TrvExit(errno, "%s %d", lua_tostring(L, -1), errno);\
    }\
} while(0);

typedef struct STScript {
    int        isSubscribeNet;
    sds        basedir;
    IniSection *iniSection;
    lua_State  *L;
} STScript;

STScript* STNewScript(IniSection *iniSection);
void STFreeScript(void *script);

int STPrepare();

int STScriptService(STScript *script, NTSnode *sn);
int STScriptServiceCallback(NTSnode *sn);

#define STAddReplyHeader(L) \
    const char *fdstr;\
    const char *tmpstr;\
    int argc;\
    NTSnode *sn;\
\
    argc = lua_gettop(L);\
    if (argc < 3) {\
        lua_pushnumber(L, -1);\
        return 1;\
    }\
\
    fdstr = lua_tostring(L, 1);\
    sn = NTGetNTSnodeByFDS(fdstr);\
    if (NULL == sn) {\
        TrvLogI("shit %s", fdstr);\
        lua_pushnumber(L, -2);\
        return 1;\
    }\
\
    sn->lua = L;\
    tmpstr = lua_tostring(L, 2); if (0 == tmpstr) sdsclear(sn->luaCbkUrl); else sn->luaCbkUrl = sdscpy(sn->luaCbkUrl, tmpstr);\
    tmpstr = lua_tostring(L, 3); if (0 == tmpstr) sdsclear(sn->luaCbkArg); else sn->luaCbkArg = sdscpy(sn->luaCbkArg, tmpstr);

const char* STgetGlobalString(lua_State *L, char *key);
int STLogI(lua_State *L);
int STLoadView(lua_State *L);
int STNTAddReplyString(lua_State *L);
int STNTAddReplyMultiString(lua_State *L);
int STNTAddReplyRawString(lua_State *L);
int STNTConnectSnode(lua_State *L);
int STDBConnect(lua_State *L);
int STDBClose(lua_State *L);
int STDBQuery(lua_State *L);
int STUILoadPage(lua_State *L);

#endif
