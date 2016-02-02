#include <stdlib.h>

#include "core/util.h"
#include "net/networking.h"
#include "service/service.h"
#include "script/script.h"

#include "g_extern.h"
#include "service/extern.h"

void SVScript(NTSnode *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat) return;

    int errno;

    STScript *script;
    listIter *li;
    listNode *ln;
    li = listGetIterator(sv_scriptCmdSubscriber, AL_START_HEAD);
    while (NULL != (ln = listNext(li))) {
        script = (STScript*)listNodeValue(ln);

        errno = STScriptService(script, sn);

        if(LUA_SERVICE_ERRNO_OK != errno) {
            NTAddReplyError(sn, "script error");
            sn->flags = SNODE_CLOSE_AFTER_REPLY;
            NTSnodeServiceSetFinishedFlag(sn);
            break;
        }
    }

    NTSnodeServiceSetFinishedFlag(sn);
}

void SVScriptCallback(NTSnode *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat) return;

    if (sdslen(sn->luaCbkUrl) > 0 && 0 != sn->lua) {
        int errno;
        errno = STScriptServiceCallback(sn);
        sn->lua = 0;
        sdsclear(sn->luaCbkUrl);
        sdsclear(sn->luaCbkArg);

        if(LUA_SERVICE_ERRNO_OK != errno) {
            NTAddReplyError(sn, "script callback error");
        }
    }

    NTSnodeServiceSetFinishedFlag(sn);
}
