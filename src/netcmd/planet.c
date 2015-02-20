#include <stdlib.h>

#include "core/util.h"
#include "net/networking.h"
#include "netcmd/netcmd.h"
#include "script/planet.h"

/* 向星球发送命令请求
 * argv[0]
 */
void planetCommand(struct NTSnode_s *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat) return;

    int errno;
    errno = STCallPlanetFunc(sn);

    if(PLANNET_LUA_CALL_ERRNO_OK != errno) {
        NTAddReplyError(sn, "planet command error");
        sn->flags = SNODE_CLOSE_AFTER_REPLY;
        setNTSnodeExcuteCommandFinished(sn);
        return;
    }

    setNTSnodeExcuteCommandFinished(sn);
}
