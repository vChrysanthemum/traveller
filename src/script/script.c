#include "core/util.h"
#include "core/config.h"
#include "core/zmalloc.h"
#include "script/script.h"
#include "net/networking.h"
#include "service/service.h"
#include "ui/ui.h"
#include "g_extern.h"

int STLuaService(lua_State *L, NTSnode *sn) {
    int errno;
    char **argv = &(sn->argv[1]);
    int argc = sn->argc - 1;
    char *funcName = *argv;
    int _m;

    lua_getglobal(L, "ServiceRouter");
    lua_pushstring(L, sn->fdstr);
    lua_pushstring(L, funcName);

    /* 从argv[1]开始，既忽略funcName */
    for (_m = 1; _m < argc; _m++) {
        lua_pushstring(L, argv[_m]);
    }

    errno = lua_pcall(L, argc+1, 0, 0);

    if (errno) {
        TrvLogW("%s", lua_tostring(L, -1));
        return LUA_SERVICE_ERRNO_INNERERR;
    }

    return LUA_SERVICE_ERRNO_OK;
}

int STLuaSrvCallback(NTSnode *sn) {
    lua_getglobal(sn->lua, "SrvCallbackRouter");
    lua_pushstring(sn->lua, sn->fdstr);
    lua_pushstring(sn->lua, sn->luaCbkUrl);
    if (0 == sdslen(sn->luaCbkArg)) {
        lua_pushnil(sn->lua);
    } else {
        lua_pushstring(sn->lua, sn->luaCbkArg);
    }

    lua_pushnumber(sn->lua, sn->recvType);
    int _m;
    for (_m = 0; _m < sn->argvSize; _m++) {
        lua_pushstring(sn->lua, sn->argv[_m]);
    }

    return lua_pcall(sn->lua, 3 + 1+sn->argvSize, 0, 0);
}

static void STInitLua(lua_State **L, char *dir) {
    *L = luaL_newstate();
    luaL_openlibs(*L);
    /*
    luaopen_base(*L);
    luaopen_table(*L);
    luaopen_string(*L);
    luaopen_math(*L);
    */

    lua_pushstring(*L, dir);
    lua_setglobal(*L, "g_basedir");

    lua_register(*L, "LogI",                    STLogI);
    lua_register(*L, "LoadView",                STLoadView);
    lua_register(*L, "NTConnectNTSnode",        STConnectNTSnode);
    lua_register(*L, "NTAddReplyString",        STAddReplyString);
    lua_register(*L, "NTAddReplyRawString",     STAddReplyRawString);
    lua_register(*L, "NTAddReplyMultiString",   STAddReplyMultiString);
    lua_register(*L, "DBQuery", STDBQuery);
}

#include "script/client.c"
#include "script/server.c"

/* 基础部分初始化 */
int STPrepare() {
    struct configOption *confOpt;
    char tmpstr[ALLOW_PATH_SIZE] = {""};

    /* 游戏服务端初始化 */
    confOpt = configGet(g_conf, "galaxies_server", "relative_path");
    if (confOpt) {

        if (confOpt->valueLen > ALLOW_PATH_SIZE) {
            TrvExit(0, "星系文件地址太长");
        }
        confOptToStr(confOpt, tmpstr);
        snprintf(g_srvGalaxydir, ALLOW_PATH_SIZE, "%s/../galaxies/%s", g_basedir, tmpstr);
        PrepareServer();
    }

    /* 游戏客户端初始化 */
    confOpt = configGet(g_conf, "galaxies_client", "relative_path");
    if (confOpt) {

        if (confOpt->valueLen > ALLOW_PATH_SIZE) {
            TrvExit(0, "星系文件地址太长");
        }
        confOptToStr(confOpt, tmpstr);
        snprintf(g_cliGalaxydir, ALLOW_PATH_SIZE, "%s/../galaxies/%s", g_basedir, tmpstr);
        PrepareClient();
    }

    return ERRNO_OK;
}
