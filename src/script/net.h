#ifndef __PLANNET_NET_H
#define __PLANNET_NET_H

#include "lua.h"

#define planetAddReplyHeader() \
    const char *fds;\
    int argc;\
    Snode *sn;\
\
    argc = lua_gettop(g_planetLuaSt);\
    if (argc < 2) {\
        lua_pushnumber(g_planetLuaSt, -1);\
        return 1;\
    }\
\
    fds = lua_tostring(g_planetLuaSt, 1);\
    sn = getSnodeByFDS(fds);\
    if (NULL == sn) {\
        lua_pushnumber(g_planetLuaSt, -2);\
    }

int planetAddReplyString(lua_State *L);
int planetAddReplyMultiString(lua_State *L);
int planetAddReplyRawString(lua_State *L);
int planetConnectSnode(lua_State *L);

#endif
