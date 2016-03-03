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

#include "net/extern.h"

list *sv_scriptServiceSubscriber;

dictType serviceTableDictType = {
    dictSdsCaseHash,           /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictSdsKeyCaseCompare,     /* key compare */
    dictSdsDestructor,         /* key destructor */
    0                       /* val destructor */
};

svServiceRouter_t svServiceRouter_tTable[] = {
    {"script",      SV_Script, 0},
    {"scriptcbk",   SV_ScriptCallback, 0},
    {"test",        SV_Test, 0},
    {"msg",         SV_Msg, 0},
    {"close",       SV_Close, 0}
};

void SV_SubscribeScriptService(stScript_t *script) {
    listAddNodeTail(sv_scriptServiceSubscriber, script);
}

void SV_Prepare() {
    sv_scriptServiceSubscriber = listCreate();

    int loopJ, tmpsize;

    nt_server.services = dictCreate(&serviceTableDictType, NULL);
    tmpsize = sizeof(svServiceRouter_tTable) / sizeof(svServiceRouter_tTable[0]);
    for (loopJ = 0; loopJ < tmpsize; loopJ++) {
        dictAdd(nt_server.services, sdsnew(svServiceRouter_tTable[loopJ].key), svServiceRouter_tTable[loopJ].proc);
    }
}
