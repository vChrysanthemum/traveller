#include <pthread.h>

#include "core/util.h"
#include "core/config.h"
#include "core/zmalloc.h"
#include "script/planet.h"
#include "script/db.h"
#include "script/net.h"
#include "net/networking.h"
#include "netcmd/netcmd.h"
#include "ui/ui.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

extern struct config *g_conf;

extern lua_State *g_srvLuaSt;
extern sqlite3 *g_srvDB;
extern char g_srvPlanetdir[ALLOW_PATH_SIZE];

extern lua_State *g_cliLuaSt;
extern sqlite3 *g_cliDB;
extern char g_cliPlanetdir[ALLOW_PATH_SIZE];
extern NTSnode *g_planetSrvSnode; /* 星球服务端连接 */

extern int g_blockCmdFd;
extern pthread_mutex_t g_blockCmdMtx;

/* 基础部分初始化 */
static void STInit(lua_State **L, char *dir) {
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
void STServerInit() {
    STInit(&g_srvLuaSt, g_srvPlanetdir);

    char *filepath = (char *)zmalloc(ALLOW_PATH_SIZE);
    memset(filepath, 0, ALLOW_PATH_SIZE);

    snprintf(filepath, ALLOW_PATH_SIZE, "%s/main.lua", g_srvPlanetdir);
    trvLogI("%s", filepath);
    errno = luaL_loadfile(g_srvLuaSt, filepath);
    if (errno) {
        trvExit(0, "%s", lua_tostring(g_srvLuaSt, -1));
    }

    memset(filepath, 0, ALLOW_PATH_SIZE);

    snprintf(filepath, ALLOW_PATH_SIZE, "%s/sqlite.db", g_srvPlanetdir);
    g_srvDB = STInitDB(filepath);


    //初始化
    errno = lua_pcall(g_srvLuaSt, 0, 0, 0);
    if (errno) {
        trvExit(0, "%s", lua_tostring(g_srvLuaSt, -1));
    }

    //调用init函数  
    lua_getglobal(g_srvLuaSt, "init");
    if (!lua_isfunction(g_srvLuaSt, -1)) {
        trvExit(0, "lua: 找不到init函数");
    }
    lua_pcall(g_srvLuaSt, 0, 0, 0);

    zfree(filepath);
}


/* 简单地对 UIInit 封装，只是为了 pthread */
static void* _UIInit(void* ptr) {
    UIInit();
    return NULL;
}

/* 客户端模式初始化 */
void STClientInit() {
    STInit(&g_cliLuaSt, g_cliPlanetdir);
    char tmpstr[64];
    char planetSrvHost[128];
    int planetSrvPort;
    struct configOption *confOpt;
    pthread_t ntid;
    char *email = "j@ioctl.cc";

    confOpt = configGet(g_conf, "planet_client", "planet_server_host");
    if (NULL == confOpt) {
        trvExit(0, "请配置星球地址，[planet_client] planet_server_host");
    }
    confOptToStr(confOpt, planetSrvHost);

    confOpt = configGet(g_conf, "planet_client", "planet_server_port");
    if (NULL == confOpt) {
        trvExit(0, "请配置星球监听端口，[planet_client] planet_server_port");
    }
    confOptToInt(confOpt, tmpstr, planetSrvPort);

    g_planetSrvSnode = NTConnectNTSnode(planetSrvHost, planetSrvPort);
    if (NULL == g_planetSrvSnode) {
        trvExit(0, "连接星球失败");
    }
    trvLogI("连接星球成功 %d", g_planetSrvSnode->fd);

    //NTPrepareBlockCmd(g_planetSrvSnode);
    //STLoginPlanet(email, "traveller");
    

    trvLogI("finshed");


    pthread_create(&ntid, NULL, _UIInit, NULL);
}
