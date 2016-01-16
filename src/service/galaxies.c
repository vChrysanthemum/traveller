#include <stdlib.h>

#include "core/util.h"
#include "net/networking.h"
#include "service/service.h"
#include "script/galaxies.h"

/* 向星系发送命令请求
 * argv[0]
 */
void SVGalaxies(NTSnode *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat) return;

    int errno;
    errno = STCallGalaxyFunc(sn);

    if(GALAXIES_LUA_CALL_ERRNO_OK != errno) {
        NTAddReplyError(sn, "galaxies service error");
        sn->flags = SNODE_CLOSE_AFTER_REPLY;
        NTSnodeServiceSetFinishedFlag(sn);
        return;
    }

    NTSnodeServiceSetFinishedFlag(sn);
}
