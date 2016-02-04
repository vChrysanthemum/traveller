#include <stdlib.h>

#include "core/util.h"
#include "core/sds.h"
#include "core/zmalloc.h"
#include "script/script.h"

#include "lua.h"

// for lua
// 请求远程机器的 ScriptService 
// from lua
// @param ConnectId         string
// @param ScriptServicePath string
// @param CallbackUrl       string     接受到远程服务器结果后的回调函数
// @param CallbackArg       string     回调函数所需参数
// @param Data              string...
// to net
// @param script            string
// @param ScriptServicePath string
// @param RequestId         string
// @param Data              string...
int STNTScriptServiceRequest(lua_State *L) {
    STAddReplyHeader(L);

    NTScriptServiceRequestCtx *ctx = NTNewScriptServiceRequestCtx();
    if (sn->scriptServiceRequestCtxListMaxId > 9999999) {
        sn->scriptServiceRequestCtxListMaxId = 0;
    }
    ctx->requestId = sn->scriptServiceRequestCtxListMaxId;
    const char *tmpstr;
    tmpstr = lua_tostring(L, 2); if (0 != tmpstr) ctx->ScriptServiceCallbackUrl = sdscpy(ctx->ScriptServiceCallbackUrl, tmpstr);
    tmpstr = lua_tostring(L, 3); if (0 != tmpstr) ctx->ScriptServiceCallbackArg = sdscpy(ctx->ScriptServiceCallbackArg, tmpstr);
    ctx->ScriptServiceLua = L;

    sn->scriptServiceRequestCtxListMaxId++;
    sn->scriptServiceRequestCtxList = listAddNodeTail(sn->scriptServiceRequestCtxList, ctx);

    char **replyArr = (char**)zmalloc(sizeof(char**) * (argc-3+2));

    replyArr[0] = "script";
    replyArr[1] = (char*)lua_tostring(L, 2);

    char requestIdStr[25];
    itoa(ctx->requestId, requestIdStr);
    replyArr[2] = requestIdStr;

    for (int loopJ = 5; loopJ <= argc; loopJ++) {
        replyArr[loopJ-2] = (char *)lua_tostring(L, loopJ);
    }

    NTAddReplyStringArgv(sn, argc-1, replyArr);

    lua_pushnumber(L, 0);

    zfree(replyArr);

    return 1;
}

// for lua
// ScriptService回应远程机器
// from lua
// @param ConnectId     string
// @param RequestId     string
// @param Data          string...
// to net
// @param scriptcbk     string
// @param RequestId     string
// @param Data          string...
int STNTScriptServiceResponse(lua_State *L) {
    STAddReplyHeader(L);

    char **replyArr = (char**)zmalloc(sizeof(char**) * (argc-1+1));

    replyArr[0] = "scriptcbk";
    replyArr[1] = (char*)lua_tostring(L, 2);

    for (int loopJ = 3; loopJ <= argc; loopJ++) {
        replyArr[loopJ-1] = (char *)lua_tostring(L, loopJ);
    }

    NTAddReplyStringArgv(sn, argc, replyArr);

    lua_pushnumber(L, 0);

    zfree(replyArr);

    return 1;
}

// for lua
// 传输字符串
// @param connectid     string
// @param data          string
int STNTAddReplyString(lua_State *L) {
    STAddReplyHeader(L);
    char *replyStr;
    
    replyStr = (char *)lua_tostring(L, 2);
    NTAddReplyString(sn, replyStr);

    lua_pushnumber(L, 0);
    return 1;
}

// for lua
// 传输多个字符串
// @param connectid     string
// @param argv...       string...
int STNTAddReplyMultiString(lua_State *L) {
    STAddReplyHeader(L);

    char **replyArr;
    int loopJ;

    //argv[0] 是 sn->fdstr

    replyArr = (char **)zmalloc(sizeof(char **) * (argc-1));

    for (loopJ = 2; loopJ <= argc; loopJ++) {
        replyArr[loopJ-2] = (char *)lua_tostring(L, loopJ);
    }

    NTAddReplyStringArgv(sn, (argc-1), replyArr);

    lua_pushnumber(L, 0);

    zfree(replyArr);

    return 1;
}

// for lua
// 传输raw字符串
// @param connectid     string
// @param data          string
int STNTAddReplyRawString(lua_State *L) {
    STAddReplyHeader(L);
    char *replyStr;
     
    replyStr = (char *)lua_tostring(L, 2);
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
