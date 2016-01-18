#include "core/util.h"
#include "core/zmalloc.h"
#include "script/galaxies.h"
#include "script/net.h"

#include "lua.h"

int STAddReplyString(lua_State *L) {
    STAddReplyHeader();
    char *replyStr;
    
    replyStr = (char *)lua_tostring(L, 2);
    NTAddReplyString(sn, replyStr);

    lua_pushnumber(L, 0);
    return 1;
}


int STAddReplyMultiString(lua_State *L) {
    STAddReplyHeader();

    char **replyArr;
    int loopJ;

    //argv[0] 是 sn->fds

    replyArr = (char **)zmalloc(sizeof(char **) * (argc-1));

    for (loopJ = 2; loopJ <= argc; loopJ++) {
        replyArr[loopJ-2] = (char *)lua_tostring(L, loopJ);
    }

    NTAddReplyStringArgv(sn, (argc-1), replyArr);

    lua_pushnumber(L, 0);

    zfree(replyArr);

    return 1;
}


int STAddReplyRawString(lua_State *L) {
    STAddReplyHeader();
    char *replyStr;
     
    replyStr = (char *)lua_tostring(L, 2);
    NTAddReplyRawString(sn, replyStr);

    lua_pushnumber(L, 0);
    return 1;
}


int STConnectNTSnode(lua_State *L) {
    char *host;
    int port;
    NTSnode *sn;
    
    host = (char *)lua_tostring(L, 1);
    port = (int)lua_tonumber(L, 2);

    sn = NTConnectNTSnode(host, port);

    if (NULL == sn) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, sn->fds);
    }

    return 1;
}
