#include "core/util.h"
#include "core/zmalloc.h"
#include "plannet/plannet.h"
#include "plannet/net.h"

#include "lua.h"

extern lua_State *g_plannetLuaSt;

int plannetAddReplyString(lua_State *L) {
    plannetAddReplyHeader();
    char *replyStr;
    
    replyStr = (char *)lua_tostring(g_plannetLuaSt, 2);
    addReplyString(sn, replyStr);

    lua_pushnumber(g_plannetLuaSt, 0);
    return 1;
}


int plannetAddReplyMultiString(lua_State *L) {
    plannetAddReplyHeader();

    char **replyArr;
    int loopJ;

    //argv[0] 是 sn->fds

    replyArr = (char **)zmalloc(sizeof(char **) * (argc-1));

    for (loopJ = 2; loopJ <= argc; loopJ++) {
        replyArr[loopJ-2] = (char *)lua_tostring(g_plannetLuaSt, loopJ);
    }

    addReplyStringArgv(sn, (argc-1), replyArr);

    lua_pushnumber(g_plannetLuaSt, 0);

    zfree(replyArr);

    return 1;
}


int plannetAddReplyRawString(lua_State *L) {
    plannetAddReplyHeader();
    char *replyStr;
     
    replyStr = (char *)lua_tostring(g_plannetLuaSt, 2);
    addReplyRawString(sn, replyStr);

    lua_pushnumber(g_plannetLuaSt, 0);
    return 1;
}


int plannetConnectSnode(lua_State *L) {
    char *host;
    int port;
    Snode *sn;
    
    host = (char *)lua_tostring(g_plannetLuaSt, 1);
    port = (int)lua_tonumber(g_plannetLuaSt, 2);

    sn = connectSnode(host, port);

    if (NULL == sn) {
        lua_pushnil(g_plannetLuaSt);
    }
    else {
        lua_pushstring(g_plannetLuaSt, sn->fds);
    }

    return 1;
}
