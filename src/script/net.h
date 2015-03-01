#ifndef __SCRIPT_NET_H
#define __SCRIPT_NET_H

#include "lua.h"

#define STAddReplyHeader() \
    const char *fds;\
    int argc;\
    NTSnode *sn;\
\
    argc = lua_gettop(L);\
    if (argc < 2) {\
        lua_pushnumber(L, -1);\
        return 1;\
    }\
\
    fds = lua_tostring(L, 1);\
    sn = NTGetNTSnodeByFDS(fds);\
    if (NULL == sn) {\
        lua_pushnumber(L, -2);\
    }

int STAddReplyString(lua_State *L);
int STAddReplyMultiString(lua_State *L);
int STAddReplyRawString(lua_State *L);
int STConnectNTSnode(lua_State *L);

#endif
