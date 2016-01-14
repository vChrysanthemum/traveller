#include <stdlib.h>

#include "core/util.h"
#include "net/networking.h"
#include "netcmd/netcmd.h"

void testCommand(NTSnode *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat) return;

    NTAddReplyMultiString(sn, 3, "osdinc", "21oi4", "oaiwef");
    setNTSnodeExcuteCommandFinished(sn);
 
    NTSnode *new_sn = NTConnectNTSnode("127.0.0.1", 1091);
    if (NULL != new_sn) {
        NTAddReplyMultiString(new_sn, 2, "close", "so;iafnonaioient");
    }
}

void msgCommand(NTSnode *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat) return;

    TrvLogD("recv msg [%d]:%s", sn->fd, sn->argv[1]);
    setNTSnodeExcuteCommandFinished(sn);
}

void closeCommand(NTSnode *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat) return;

    TrvLogD("closed by other people");
    NTAddReplyMultiString(sn, 2, "msg", "21oi4oaiwef");
    setNTSnodeExcuteCommandFinished(sn);
 
    sn->flags |= SNODE_CLOSE_AFTER_REPLY;

}
