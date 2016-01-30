#include <stdlib.h>

#include "core/util.h"
#include "net/networking.h"
#include "service/service.h"
#include "script/script.h"

#include "g_extern.h"
#include "service/extern.h"

/* 星系接受命令
 * argv[0]
 */
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
            NTAddReplyError(sn, "script service error");
            sn->flags = SNODE_CLOSE_AFTER_REPLY;
            NTSnodeServiceSetFinishedFlag(sn);
            break;
        }
    }

    NTSnodeServiceSetFinishedFlag(sn);
}
