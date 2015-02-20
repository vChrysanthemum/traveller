#include "core/util.h"
#include "core/zmalloc.h"
#include "script/planet.h"
#include "script/net.h"

#include "lua.h"

extern lua_State *g_planetLuaSt;

int STAddReplyString(lua_State *L) {
    STAddReplyHeader();
    char *replyStr;
    
    replyStr = (char *)lua_tostring(g_planetLuaSt, 2);
    NTAddReplyString(sn, replyStr);

    lua_pushnumber(g_planetLuaSt, 0);
    return 1;
}


int STAddReplyMultiString(lua_State *L) {
    STAddReplyHeader();

    char **replyArr;
    int loopJ;

    //argv[0] 是 sn->fds

    replyArr = (char **)zmalloc(sizeof(char **) * (argc-1));

    for (loopJ = 2; loopJ <= argc; loopJ++) {
        replyArr[loopJ-2] = (char *)lua_tostring(g_planetLuaSt, loopJ);
    }

    NTAddReplyStringArgv(sn, (argc-1), replyArr);

    lua_pushnumber(g_planetLuaSt, 0);

    zfree(replyArr);

    return 1;
}


int STAddReplyRawString(lua_State *L) {
    STAddReplyHeader();
    char *replyStr;
     
    replyStr = (char *)lua_tostring(g_planetLuaSt, 2);
    NTAddReplyRawString(sn, replyStr);

    lua_pushnumber(g_planetLuaSt, 0);
    return 1;
}


int STConnectNTSnode(lua_State *L) {
    char *host;
    int port;
    NTSnode *sn;
    
    host = (char *)lua_tostring(g_planetLuaSt, 1);
    port = (int)lua_tonumber(g_planetLuaSt, 2);

    sn = NTConnectNTSnode(host, port);

    if (NULL == sn) {
        lua_pushnil(g_planetLuaSt);
    }
    else {
        lua_pushstring(g_planetLuaSt, sn->fds);
    }

    return 1;
}
