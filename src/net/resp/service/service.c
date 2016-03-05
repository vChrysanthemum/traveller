#include "lua.h"

#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/util.h"
#include "core/ini.h"
#include "core/extern.h"

#include "net/networking.h"
#include "script/script.h"
#include "net/resp/service/service.h"

#include "net/extern.h"

list *nt_respSVScriptServiceSubscriber;

dictType serviceTableDictType = {
    dictSdsCaseHash,           /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictSdsKeyCaseCompare,     /* key compare */
    dictSdsDestructor,         /* key destructor */
    0                       /* val destructor */
};

ntRespSVServiceRouter_t ntRespSVServiceRouter_tTable[] = {
    {"script",      NTRespSV_Script, 0},
    {"scriptcbk",   NTRespSV_ScriptCallback, 0},
    {"test",        NTRespSV_Test, 0},
    {"msg",         NTRespSV_Msg, 0},
    {"close",       NTRespSV_Close, 0}
};

void NTRespSV_SubscribeScriptService(stScript_t *script) {
    listAddNodeTail(nt_respSVScriptServiceSubscriber, script);
}

void NTRespSV_Prepare() {
    nt_respSVScriptServiceSubscriber = listCreate();

    int loopJ, tmpsize;

    nt_RespServer.services = dictCreate(&serviceTableDictType, NULL);
    tmpsize = sizeof(ntRespSVServiceRouter_tTable) / sizeof(ntRespSVServiceRouter_tTable[0]);
    for (loopJ = 0; loopJ < tmpsize; loopJ++) {
        dictAdd(nt_RespServer.services, sdsnew(ntRespSVServiceRouter_tTable[loopJ].key), ntRespSVServiceRouter_tTable[loopJ].proc);
    }
}
