#include "core/util.h"
#include "core/config.h"
#include "core/zmalloc.h"
#include "script/script.h"
#include "script/galaxies.h"
#include "net/networking.h"
#include "service/service.h"
#include "ui/ui.h"
#include "g_extern.h"

int STLuaCallback(NTSnode *sn) {
    lua_getglobal(sn->lua, "SrvCallbackRouter");
    lua_pushstring(sn->lua, sn->fdstr);
    lua_pushstring(sn->lua, sn->lua_cbk_url);
    if (0 == sdslen(sn->lua_cbk_arg)) {
        lua_pushnil(sn->lua);
    } else {
        lua_pushstring(sn->lua, sn->lua_cbk_arg);
    }
    return lua_pcall(sn->lua, 3, 0, 0);
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
