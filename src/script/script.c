#include "core/util.h"
#include "core/config.h"
#include "core/zmalloc.h"
#include "script/script.h"
#include "script/galaxies.h"
#include "script/db.h"
#include "script/net.h"
#include "net/networking.h"
#include "service/service.h"
#include "ui/ui.h"
#include "g_extern.h"

static void STInitLua(lua_State **L, char *dir) {
    int errno;

    *L = luaL_newstate();
    luaL_openlibs(*L);
    /*
    luaopen_base(*L);
    luaopen_table(*L);
    luaopen_string(*L);
    luaopen_math(*L);
    */

    lua_pushstring(*L, dir);
    lua_setglobal(*L, "basedir");

    lua_register(*L, "NTConnectNTSnode", STConnectNTSnode);
    lua_register(*L, "NTAddReplyString", STAddReplyString);
    lua_register(*L, "NTAddReplyRawString", STAddReplyRawString);
    lua_register(*L, "NTAddReplyMultiString", STAddReplyMultiString);
    lua_register(*L, "DBQuery", STDBQuery);

}

/* 服务端模式初始化 */
static void PrepareServer() {
    STInitLua(&g_srvLuaSt, g_srvGalaxydir);

    char *filepath = (char *)zmalloc(ALLOW_PATH_SIZE);
    memset(filepath, 0, ALLOW_PATH_SIZE);

    snprintf(filepath, ALLOW_PATH_SIZE, "%s/main.lua", g_srvGalaxydir);
    TrvLogI("%s", filepath);
    errno = luaL_loadfile(g_srvLuaSt, filepath);
    if (errno) {
        TrvExit(0, "%s", lua_tostring(g_srvLuaSt, -1));
    }

    memset(filepath, 0, ALLOW_PATH_SIZE);

    snprintf(filepath, ALLOW_PATH_SIZE, "%s/sqlite.db", g_srvGalaxydir);
    g_srvDB = STInitDB(filepath);


    //初始化
    errno = lua_pcall(g_srvLuaSt, 0, 0, 0);
    if (errno) {
        TrvExit(0, "%s", lua_tostring(g_srvLuaSt, -1));
    }

    //调用init函数  
    lua_getglobal(g_srvLuaSt, "init");
    if (!lua_isfunction(g_srvLuaSt, -1)) {
        TrvExit(0, "lua: 找不到init函数");
    }
    lua_pcall(g_srvLuaSt, 0, 0, 0);

    zfree(filepath);
}

/* 客户端模式初始化 */
static void PrepareClient() {
    STInitLua(&g_cliLuaSt, g_cliGalaxydir);
    char tmpstr[64];
    char galaxiesSrvHost[128];
    int galaxiesSrvPort;
    struct configOption *confOpt;

    confOpt = configGet(g_conf, "galaxies_client", "galaxies_server_host");
    if (NULL == confOpt) {
        TrvExit(0, "请配置星系地址，[galaxies_client] galaxies_server_host");
    }
    confOptToStr(confOpt, galaxiesSrvHost);

    confOpt = configGet(g_conf, "galaxies_client", "galaxies_server_port");
    if (NULL == confOpt) {
        TrvExit(0, "请配置星系监听端口，[galaxies_client] galaxies_server_port");
    }
    confOptToInt(confOpt, tmpstr, galaxiesSrvPort);

    g_galaxiesSrvSnode = NTConnectNTSnode(galaxiesSrvHost, galaxiesSrvPort);
    if (NULL == g_galaxiesSrvSnode) {
        TrvExit(0, "连接星系失败");
    }
    TrvLogI("连接星系成功 %d", g_galaxiesSrvSnode->fd);

    char *email = "j@ioctl.cc";
    STLoginGalaxy(email, "traveller");

    TrvLogI("finshed");
}

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
