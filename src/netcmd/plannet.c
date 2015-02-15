#include <stdlib.h>

#include "core/util.h"
#include "net/networking.h"
#include "netcmd/netcmd.h"
#include "plannet/plannet.h"

/* 向星球发送命令请求
 * argv[0]
 */
void plannetCommand(struct Snode_s *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat) return;

    int errno;
    errno = callPlannet(sn);

    if(PLANNET_LUA_CALL_ERRNO_OK != errno) {
        addReplyError(sn, "plannet command error");
        sn->flags = SNODE_CLOSE_AFTER_REPLY;
        setSnodeExcuteCommandFinished(sn);
        return;
    }

    setSnodeExcuteCommandFinished(sn);
}
