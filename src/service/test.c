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

void SV_Test(ntSnode_t *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat) return;

    NT_AddReplyMultiString(sn, 3, "osdinc", "21oi4", "oaiwef");
    NT_SnodeServiceSetFinishedFlag(sn);
 
    ntSnode_t *new_sn = NT_ConnectSnode("127.0.0.1", 1091);
    if (0 != new_sn) {
        NT_AddReplyMultiString(new_sn, 2, "close", "so;iafnonaioient");
    }
}

void SV_Msg(ntSnode_t *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat) return;

    TrvLogD("recv msg [%d]:%s", sn->fd, sn->argv[1]);
    NT_SnodeServiceSetFinishedFlag(sn);
}

void SV_Close(ntSnode_t *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat) return;

    TrvLogD("closed by other people");
    NT_AddReplyMultiString(sn, 2, "msg", "21oi4oaiwef");
    NT_SnodeServiceSetFinishedFlag(sn);
 
    sn->flags |= SNODE_CLOSE_AFTER_REPLY;

}
