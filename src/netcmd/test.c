#include <stdlib.h>

#include "core/util.h"
#include "net/networking.h"
#include "netcmd/netcmd.h"

void testCommand(struct NTSnode_s *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat) return;

    NTAddReplyMultiString(sn, 3, "osdinc", "21oi4", "oaiwef");
    setNTSnodeExcuteCommandFinished(sn);
 
    /*   
    NTSnode *new_sn = NTConnectNTSnode("127.0.0.1", 1091);
    if (NULL != new_sn) {
        NTAddReplyMultiString(new_sn, 2, "close", "so;iafnonaioient");
    }
    */
}

void closeCommand(struct NTSnode_s *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat) return;

    NTFreeNTSnode(sn);
    trvLogI("closed by other people");
    exit(-1);
}
