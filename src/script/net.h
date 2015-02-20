#ifndef __PLANNET_NET_H
#define __PLANNET_NET_H

#include "lua.h"

#define STAddReplyHeader() \
    const char *fds;\
    int argc;\
    NTSnode *sn;\
\
    argc = lua_gettop(g_planetLuaSt);\
    if (argc < 2) {\
        lua_pushnumber(g_planetLuaSt, -1);\
        return 1;\
    }\
\
    fds = lua_tostring(g_planetLuaSt, 1);\
    sn = NTGetNTSnodeByFDS(fds);\
    if (NULL == sn) {\
        lua_pushnumber(g_planetLuaSt, -2);\
    }

int STAddReplyString(lua_State *L);
int STAddReplyMultiString(lua_State *L);
int STAddReplyRawString(lua_State *L);
int STConnectNTSnode(lua_State *L);

#endif
