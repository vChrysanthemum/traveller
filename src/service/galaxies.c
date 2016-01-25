#include <stdlib.h>

#include "core/util.h"
#include "net/networking.h"
#include "service/service.h"
#include "script/script.h"

#include "g_extern.h"

/* 星系接受命令
 * argv[0]
 */
void SVGalaxies(NTSnode *sn) {
    if (SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat) return;

    int errno;
    errno = STLuaService(g_srvLuaSt, sn);

    if(LUA_SERVICE_ERRNO_OK != errno) {
        NTAddReplyError(sn, "galaxies service error");
        sn->flags = SNODE_CLOSE_AFTER_REPLY;
        NTSnodeServiceSetFinishedFlag(sn);
        return;
    }

    NTSnodeServiceSetFinishedFlag(sn);
}
