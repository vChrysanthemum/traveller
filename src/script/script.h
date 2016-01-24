#ifndef __SCRIPT_SCRIPT_H
#define __SCRIPT_SCRIPT_H

#include "core/util.h"
#include "net/networking.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "sqlite3.h"

int STPrepare();
sqlite3* STinitDB(char *filepath);

int STLuaCallback(NTSnode *sn);

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
    tmpstr = lua_tostring(L, 2); if (0 == tmpstr) sdsclear(sn->lua_cbk_url); else sn->lua_cbk_url = sdscpy(sn->lua_cbk_url, tmpstr);\
    tmpstr = lua_tostring(L, 3); if (0 == tmpstr) sdsclear(sn->lua_cbk_arg); else sn->lua_cbk_arg = sdscpy(sn->lua_cbk_arg, tmpstr);

const char* STgetGlobalString(lua_State *L, char *key);
int STLogI(lua_State *L);
int STLoadView(lua_State *L);
int STAddReplyString(lua_State *L);
int STAddReplyMultiString(lua_State *L);
int STAddReplyRawString(lua_State *L);
int STConnectNTSnode(lua_State *L);
int STDBQuery(lua_State *L);

#endif
