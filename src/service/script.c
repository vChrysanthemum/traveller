#include <stdlib.h>

#include "lua.h"

#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/util.h"
#include "core/ini.h"
#include "core/extern.h"

#include "net/networking.h"
#include "script/script.h"
#include "service/service.h"

#include "g_extern.h"
#include "service/extern.h"

/**
 * 由service调用该函数
 * 触发脚本服务
 * from net
 * @param script             string
 * @param ScriptServicePath  string
 * @param RequestId          string
 * @param Data               string...
 * to lua
 * @param ConnectId          string
 * @param ScriptServicePath  string
 * @param RequestId          string
 * @param Data               string...
 */
static int SV_scriptService(stScript_t *script, ntSnode_t *sn) {
    int errno;
    char **argv = &(sn->argv[1]);
    int argc = sn->argc - 1; // 减去 argv[0] = "script"
    lua_State *L = script->L;

    lua_getglobal(L, "ServiceRouter");
    lua_pushstring(L, sn->fdstr);
    lua_pushstring(L, argv[0]);
    lua_pushstring(L, argv[1]);

    lua_newtable(L);

    for (int i = 2; i < argc; i += 2) {
        lua_pushstring(L, argv[i]);
        lua_pushstring(L, argv[i+1]);
        lua_settable(L, -3);
    }

    errno = lua_pcall(L, 4, 0, 0);

    if (0 != errno) {
        TrvLogW("%s", lua_tostring(L, -1));
        return SCRIPT_SERVICE_ERRNO_INNERERR;
    }

    return SCRIPT_SERVICE_ERRNO_OK;
}

/**
 * 由service调用该函数
 * 远程机器运行脚本服务结束返回结果，触发回调
 * from net
 * @param scriptcbk                string
 * @param RequestId                string
 * @param Data                     string...
 * to lua
 * @param ConnectId                string
 * @param ScriptServiceCallbackUrl string
 * @param ScriptServiceCallbackArg string
 * @param Data                     string...
 */
static int SV_scriptServiceCallback(ntSnode_t *sn) {
    int errno;
    char **argv = &(sn->argv[1]);

    int requestId = atoi(argv[0]);

    listIter *li;
    listNode *ln;
    li = listGetIterator(sn->scriptServiceRequestCtxList, AL_START_HEAD);
    ntScriptServiceRequestCtx_t *ctx, *ctxTarget = 0;
    while (0 != (ln = listNext(li))) {
        ctx = (ntScriptServiceRequestCtx_t*)listNodeValue(ln);
        if (requestId == ctx->requestId) {
            ctxTarget = ctx;
            break;
        }
    }
    listReleaseIterator(li);

    if (0 == ctxTarget) {
        return SCRIPT_SERVICE_ERRNO_INNERERR;
    }

    lua_State *L = ctxTarget->ScriptServiceLua;

    lua_getglobal(L, "ServiceCallbackRouter");

    lua_pushstring(L, sn->fdstr);
    lua_pushstring(L, ctxTarget->ScriptServiceCallbackUrl);
    if (0 == sdslen(ctxTarget->ScriptServiceCallbackArg)) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, ctxTarget->ScriptServiceCallbackArg);
    }

    lua_newtable(L);

    for (int i = 2; i < sn->argvSize; i += 2) {
        lua_pushstring(L, sn->argv[i]);
        lua_pushstring(L, sn->argv[i+1]);
        lua_settable(L, -3);
    }

    errno = lua_pcall(L, 4, 0, 0);

    listDelNode(sn->scriptServiceRequestCtxList, ln);

    if (0 != errno) {
        TrvLogW("%s", lua_tostring(L, -1));
        return SCRIPT_SERVICE_ERRNO_INNERERR;
    }

    return SCRIPT_SERVICE_ERRNO_OK;
}

/**
 * 触发脚本服务
 */
void SV_Script(ntSnode_t *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat) return;

    int errno;

    stScript_t *script;
    listIter *li;
    listNode *ln;
    li = listGetIterator(sv_scriptServiceSubscriber, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        script = (stScript_t*)listNodeValue(ln);

        errno = SV_scriptService(script, sn);

        if(SCRIPT_SERVICE_ERRNO_OK != errno) {
            NT_AddReplyError(sn, "script error");
            sn->flags = SNODE_CLOSE_AFTER_REPLY;
            NT_SnodeServiceSetFinishedFlag(sn);
            break;
        }
    }

    NT_SnodeServiceSetFinishedFlag(sn);
}

/**
 * 远程机器运行脚本服务结束返回结果，触发回调
 */
void SV_ScriptCallback(ntSnode_t *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat) return;

    int errno;
    errno = SV_scriptServiceCallback(sn);
    if(SCRIPT_SERVICE_ERRNO_OK != errno) {
        NT_AddReplyError(sn, "script callback error");
    }

    NT_SnodeServiceSetFinishedFlag(sn);
}
