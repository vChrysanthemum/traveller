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
#include "net/resp/service/service.h"

void NTRespSV_Test(ntRespSnode_t *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat) return;

    NTResp_AddReplyMultiString(sn, 3, "osdinc", "21oi4", "oaiwef");
    NTResp_SnodeServiceSetFinishedFlag(sn);
 
    ntRespSnode_t *new_sn = NTResp_ConnectSnode("127.0.0.1", 1091);
    if (0 != new_sn) {
        NTResp_AddReplyMultiString(new_sn, 2, "close", "so;iafnonaioient");
    }
}

void NTRespSV_Msg(ntRespSnode_t *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat) return;

    C_UtilLogD("recv msg [%d]:%s", sn->fd, sn->argv[1]);
    NTResp_SnodeServiceSetFinishedFlag(sn);
}

void NTRespSV_Close(ntRespSnode_t *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat) return;

    C_UtilLogD("closed by other people");
    NTResp_AddReplyMultiString(sn, 2, "msg", "21oi4oaiwef");
    NTResp_SnodeServiceSetFinishedFlag(sn);
 
    sn->flags |= SNODE_CLOSE_AFTER_REPLY;

}
