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

int STPrepare();
sqlite3* STinitDB(char *filepath);

int STLuaService(lua_State *L, NTSnode *sn);
int STLuaSrvCallback(NTSnode *sn);

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
int STAddReplyString(lua_State *L);
int STAddReplyMultiString(lua_State *L);
int STAddReplyRawString(lua_State *L);
int STConnectNTSnode(lua_State *L);
int STDBQuery(lua_State *L);

#endif
