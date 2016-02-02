#include "core/util.h"
#include "core/dict.h"
#include "net/networking.h"
#include "service/service.h"
#include "net/extern.h"

list *sv_scriptCmdSubscriber;

dictType serviceTableDictType = {
    dictSdsCaseHash,           /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictSdsKeyCaseCompare,     /* key compare */
    dictSdsDestructor,         /* key destructor */
    NULL                       /* val destructor */
};

SVServiceRouter SVServiceRouterTable[] = {
    {"script",      SVScript, 0},
    {"scriptcbk",   SVScriptCallback, 0},
    {"test",        SVTest, 0},
    {"msg",         SVMsg, 0},
    {"close",       SVClose, 0}
};

void SVSubscribeScriptCmd(STScript *script) {
    listAddNodeTail(sv_scriptCmdSubscriber, script);
}

void SVPrepare() {
    sv_scriptCmdSubscriber = listCreate();

    int loopJ, tmpsize;

    nt_server.services = dictCreate(&serviceTableDictType, NULL);
    tmpsize = sizeof(SVServiceRouterTable) / sizeof(SVServiceRouterTable[0]);
    for (loopJ = 0; loopJ < tmpsize; loopJ++) {
        dictAdd(nt_server.services, sdsnew(SVServiceRouterTable[loopJ].key), SVServiceRouterTable[loopJ].proc);
    }
}
