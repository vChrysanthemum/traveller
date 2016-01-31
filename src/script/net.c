#include "core/util.h"
#include "core/zmalloc.h"
#include "script/script.h"

#include "lua.h"

// for lua
// 传输字符串
// @param connectid     string
// @param callbackurl   string     接受到远程服务器结果后的回调函数
// @param callbackarg   string     回调函数所需参数
// @param data          string...
int STNTAddReplyString(lua_State *L) {
    STAddReplyHeader(L);
    char *replyStr;
    
    replyStr = (char *)lua_tostring(L, 1+3);
    NTAddReplyString(sn, replyStr);

    lua_pushnumber(L, 0);
    return 1;
}

// for lua
// 传输多个字符串
// @param connectid     string
// @param callbackurl   string
// @param callbackarg   string
// @param argv...       string...
int STNTAddReplyMultiString(lua_State *L) {
    STAddReplyHeader(L);

    char **replyArr;
    int loopJ;

    //argv[0] 是 sn->fdstr

    replyArr = (char **)zmalloc(sizeof(char **) * (argc-3));

    for (loopJ = 1+3; loopJ <= argc; loopJ++) {
        replyArr[loopJ-(1+3)] = (char *)lua_tostring(L, loopJ);
    }

    NTAddReplyStringArgv(sn, (argc-3), replyArr);

    lua_pushnumber(L, 0);

    zfree(replyArr);

    return 1;
}

// for lua
// 传输raw字符串
// @param connectid     string
// @param callbackurl   string
// @param callbackarg   string
// @param data          string...
int STNTAddReplyRawString(lua_State *L) {
    STAddReplyHeader(L);
    char *replyStr;
     
    replyStr = (char *)lua_tostring(L, 1+3);
    NTAddReplyRawString(sn, replyStr);

    lua_pushnumber(L, 0);
    return 1;
}


// for lua
// 连接远程机器
// @param host string  远程机器地址
// @param port number
int STNTConnectSnode(lua_State *L) {
    char *host;
    int port;
    NTSnode *sn;
    
    host = (char *)lua_tostring(L, 1);
    port = (int)lua_tonumber(L, 2);

    sn = NTConnectNTSnode(host, port);

    if (NULL == sn) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, sn->fdstr);
    }

    return 1;
}
