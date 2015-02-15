#ifndef __PLANNET_NET_H
#define __PLANNET_NET_H

#include "lua.h"

#define plannetAddReplyHeader() \
    const char *fds;\
    int argc;\
    Snode *sn;\
\
    argc = lua_gettop(g_plannetLuaSt);\
    if (argc < 2) {\
        lua_pushnumber(g_plannetLuaSt, -1);\
        return 1;\
    }\
\
    fds = lua_tostring(g_plannetLuaSt, 1);\
    sn = getSnodeByFDS(fds);\
    if (NULL == sn) {\
        lua_pushnumber(g_plannetLuaSt, -2);\
    }

int plannetAddReplyString(lua_State *L);
int plannetAddReplyMultiString(lua_State *L);
int plannetAddReplyRawString(lua_State *L);
int plannetConnectSnode(lua_State *L);

#endif
