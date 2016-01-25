/* 星系运转需要的基础C
 */
#include "core/util.h"
#include "core/zmalloc.h"
#include "net/networking.h"
#include "script/script.h"
#include "script/galaxies.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

extern NTSnode *g_galaxiesSrvSnode; /* 星系服务端连接 */

extern char g_galaxiesdir[];
extern lua_State *g_srvLuaSt;

int STCallGalaxyFunc(NTSnode *sn) {
    int errno;
    char **argv = &(sn->argv[1]);
    int argc = sn->argc - 1;
    char *funcName = *argv;
    int _m;

    lua_getglobal(g_srvLuaSt, "GalaxiesRouter");
    lua_pushstring(g_srvLuaSt, sn->fdstr);
    lua_pushstring(g_srvLuaSt, funcName);

    /* 从argv[1]开始，既忽略funcName */
    for (_m = 1; _m < argc; _m++) {
        lua_pushstring(g_srvLuaSt, argv[_m]);
    }

    //argc == count(sn->fd + argv - funcName)
    errno = lua_pcall(g_srvLuaSt, argc+1, 0, 0);

    if (errno) {
        TrvLogW("%s", lua_tostring(g_srvLuaSt, -1));
        return GALAXIES_LUA_CALL_ERRNO_FUNC502;
    }

    return GALAXIES_LUA_CALL_ERRNO_OK;
}

static void STLoginGalaxyGetResult(NTSnode *sn) {
    TrvLogW("%d %s", sn->recvType, sn->argv[0]);
}

int STLoginGalaxy(char *email, char *password) {
    NTAddReplyMultiString(g_galaxiesSrvSnode, 4, "galaxies", "PUBCitizenLogin", email, password);
    g_galaxiesSrvSnode->proc = STLoginGalaxyGetResult;
    return 0;
}
