#include <stdlib.h>

#include "core/util.h"
#include "net/networking.h"
#include "netcmd/netcmd.h"

void testCommand(struct Snode_s *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat) return;

    addReplyMultiString(sn, 3, "osdinc", "21oi4", "oaiwef");
    setSnodeExcuteCommandFinished(sn);
 
    /*   
    Snode *new_sn = connectSnode("127.0.0.1", 1091);
    if (NULL != new_sn) {
        addReplyMultiString(new_sn, 2, "close", "so;iafnonaioient");
    }
    */
}

void closeCommand(struct Snode_s *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat) return;

    freeSnode(sn);
    trvLogI("closed by other people");
    exit(-1);
}
