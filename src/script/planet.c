/* 星球运转需要的基础C
 */
#include "core/util.h"
#include "core/zmalloc.h"
#include "net/networking.h"
#include "script/planet.h"
#include "script/db.h"
#include "script/net.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


extern char g_planetdir[];
extern lua_State *g_planetLuaSt;

/* 调用星球上的函数
 * 只允许访问函数名 PUB 为开头的函数
 * lua中函数只返回一个字符串
 * argv 为 sn->argv[1:]，既忽略 sn的procName
 */
int STCallPlanetFunc(NTSnode *sn) {
    char **argv = &(sn->argv[1]);
    char *funcName = *argv;
    int argc = sn->argc - 1;
    int _m;

    if ('P' != *funcName && 'U' != *funcName && 'B' != *funcName) {
        return PLANET_LUA_CALL_ERRNO_FUNC403;
    }

    _m = lua_gettop(g_planetLuaSt);
    if (_m > 0) lua_pop(g_planetLuaSt, _m);

    lua_getglobal(g_planetLuaSt, funcName);
    if (!lua_isfunction(g_planetLuaSt, -1)) {
        trvLogW("%s not exists", funcName);
        return PLANET_LUA_CALL_ERRNO_FUNC404;
    }

    lua_pushinteger(g_planetLuaSt, sn->fd);


    /* 从argv[1]开始，既忽略funcName */
    for (_m = 1; _m < argc; _m++) {
        lua_pushstring(g_planetLuaSt, argv[_m]);
    }


    //argc == count(sn->fd + argv - funcName)
    errno = lua_pcall(g_planetLuaSt, argc, 0, 0);
    if (errno) {
        trvLogW("%s", lua_tostring(g_planetLuaSt, -1));
        lua_pop(g_planetLuaSt, _m);
        return PLANET_LUA_CALL_ERRNO_FUNC502;
    }

    lua_pop(g_planetLuaSt, lua_gettop(g_planetLuaSt));

    return PLANET_LUA_CALL_ERRNO_OK;
}
