/* 星系运转需要的基础C
 */
#include "core/util.h"
#include "core/zmalloc.h"
#include "net/networking.h"
#include "script/galaxies.h"
#include "script/db.h"
#include "script/net.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

extern NTSnode *g_galaxiesSrvSnode; /* 星系服务端连接 */

extern char g_galaxiesdir[];
extern lua_State *g_srvLuaSt;

/* 调用星系上的函数
 * 只允许访问函数名 PUB 为开头的函数
 * lua中函数只返回一个字符串
 * argv 为 sn->argv[1:]，既忽略 sn的procName
 */
int STCallGalaxyFunc(NTSnode *sn) {
    char **argv = &(sn->argv[1]);
    char *funcName = *argv;
    int argc = sn->argc - 1;
    int _m;

    if ('P' != *funcName && 'U' != *funcName && 'B' != *funcName) {
        return GALAXIES_LUA_CALL_ERRNO_FUNC403;
    }

    _m = lua_gettop(g_srvLuaSt);
    if (_m > 0) lua_pop(g_srvLuaSt, _m);

    lua_getglobal(g_srvLuaSt, funcName);
    if (!lua_isfunction(g_srvLuaSt, -1)) {
        ZeusLogW("%s not exists", funcName);
        return GALAXIES_LUA_CALL_ERRNO_FUNC404;
    }

    lua_pushinteger(g_srvLuaSt, sn->fd);


    /* 从argv[1]开始，既忽略funcName */
    for (_m = 1; _m < argc; _m++) {
        lua_pushstring(g_srvLuaSt, argv[_m]);
    }


    //argc == count(sn->fd + argv - funcName)
    errno = lua_pcall(g_srvLuaSt, argc, 0, 0);
    if (errno) {
        ZeusLogW("%s", lua_tostring(g_srvLuaSt, -1));
        lua_pop(g_srvLuaSt, _m);
        return GALAXIES_LUA_CALL_ERRNO_FUNC502;
    }

    lua_pop(g_srvLuaSt, lua_gettop(g_srvLuaSt));

    return GALAXIES_LUA_CALL_ERRNO_OK;
}


/* 玩家登录星系
 */
int STLoginGalaxy(char *email, char *password) {
    NTAddReplyMultiString(g_galaxiesSrvSnode, 4, "galaxies", "PUBCitizenLogin", email, password);
    return 0;
}
