/* 星球运转需要的基础C
 */
#include "core/util.h"
#include "core/zmalloc.h"
#include "net/networking.h"
#include "plannet/plannet.h"
#include "plannet/db.h"
#include "plannet/net.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


extern char g_plannetdir[];
extern lua_State *g_plannetLuaSt;

/* 调用星球上的函数
 * 只允许访问函数名 PUB 为开头的函数
 * lua中函数只返回一个字符串
 * argv 为 sn->argv[1:]，既忽略 sn的procName
 */
int callPlannet(Snode *sn) {
    char **argv = &(sn->argv[1]);
    char *funcName = *argv;
    int argc = sn->argc - 1;
    int _m;

    if ('P' != *funcName && 'U' != *funcName && 'B' != *funcName) {
        return PLANNET_LUA_CALL_ERRNO_FUNC403;
    }

    _m = lua_gettop(g_plannetLuaSt);
    if (_m > 0) lua_pop(g_plannetLuaSt, _m);

    lua_getglobal(g_plannetLuaSt, funcName);
    if (!lua_isfunction(g_plannetLuaSt, -1)) {
        trvLogW("%s not exists", funcName);
        return PLANNET_LUA_CALL_ERRNO_FUNC404;
    }

    lua_pushinteger(g_plannetLuaSt, sn->fd);


    /* 从argv[1]开始，既忽略funcName */
    for (_m = 1; _m < argc; _m++) {
        lua_pushstring(g_plannetLuaSt, argv[_m]);
    }


    //argc == count(sn->fd + argv - funcName)
    errno = lua_pcall(g_plannetLuaSt, argc, 0, 0);
    if (errno) {
        trvLogW("%s", lua_tostring(g_plannetLuaSt, -1));
        lua_pop(g_plannetLuaSt, _m);
        return PLANNET_LUA_CALL_ERRNO_FUNC502;
    }

    lua_pop(g_plannetLuaSt, lua_gettop(g_plannetLuaSt));

    return PLANNET_LUA_CALL_ERRNO_OK;
}


void initPlannet() {
    int errno;
    char *filepath = zmalloc(ALLOW_PATH_SIZE);
    memset(filepath, 0, ALLOW_PATH_SIZE);

    initPlannetDB();

    g_plannetLuaSt = luaL_newstate();
    luaL_openlibs(g_plannetLuaSt);
    /*
    luaopen_base(g_plannetLuaSt);
    luaopen_table(g_plannetLuaSt);
    luaopen_string(g_plannetLuaSt);
    luaopen_math(g_plannetLuaSt);
    */

    snprintf(filepath, ALLOW_PATH_SIZE, "%s/main.lua", g_plannetdir);

    lua_pushstring(g_plannetLuaSt, g_plannetdir);
    lua_setglobal(g_plannetLuaSt, "basedir");

    errno = luaL_loadfile(g_plannetLuaSt, filepath);
    if (errno) {
        trvExit(0, "%s", lua_tostring(g_plannetLuaSt, -1));
    }

    lua_register(g_plannetLuaSt, "NTConnectSnode", plannetConnectSnode);
    lua_register(g_plannetLuaSt, "NTAddReplyString", plannetAddReplyString);
    lua_register(g_plannetLuaSt, "NTAddReplyRawString", plannetAddReplyRawString);
    lua_register(g_plannetLuaSt, "NTAddReplyMultiString", plannetAddReplyMultiString);
    lua_register(g_plannetLuaSt, "DBQuery", plannetDBQuery);

    //初始化
    errno = lua_pcall(g_plannetLuaSt, 0, 0, 0);
    if (errno) {
        trvExit(0, "%s", lua_tostring(g_plannetLuaSt, -1));
    }

    //调用init函数  
    lua_getglobal(g_plannetLuaSt, "init");
    if (!lua_isfunction(g_plannetLuaSt, -1)) {
        trvExit(0, "lua: 找不到init函数");
    }
    lua_pcall(g_plannetLuaSt, 0, 0, 0);

    zfree(filepath);

}
