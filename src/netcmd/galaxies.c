#include <stdlib.h>
#include <pthread.h>


#include "core/util.h"
#include "net/networking.h"
#include "netcmd/netcmd.h"
#include "script/galaxies.h"

/* 向星系发送命令请求
 * argv[0]
 */
void galaxiesCommand(struct NTSnode_s *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat) return;

    int errno;
    errno = STCallGalaxyFunc(sn);

    if(GALAXIES_LUA_CALL_ERRNO_OK != errno) {
        NTAddReplyError(sn, "galaxies command error");
        sn->flags = SNODE_CLOSE_AFTER_REPLY;
        setNTSnodeExcuteCommandFinished(sn);
        return;
    }

    setNTSnodeExcuteCommandFinished(sn);
}
