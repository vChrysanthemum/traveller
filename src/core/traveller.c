#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

#include "lua.h"
#include "sqlite3.h"
#include "core/config.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "net/networking.h"
#include "net/ae.h"
#include "script/planet.h"
#include "ui/ui.h"

#define TMPSTR_SIZE ALLOW_PATH_SIZE

/* 全局变量
 */
struct NTServer g_server;
char g_basedir[ALLOW_PATH_SIZE] = {""}; /* 绝对路径为 $(traveller)/src */
char g_planetdir[ALLOW_PATH_SIZE] = {""}; /* 需要加载的星球路径 */
lua_State *g_planetLuaSt;
sqlite3 *g_planetDB;
UIWin *g_rootUIWin;
UICursor g_cursor;


void beforeSleep(struct aeEventLoop *eventLoop) {
    NOTUSED(eventLoop);
}

int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData) {
    g_server.unixtime = time(NULL);
    trvLogI("serverCron");
    return 0;
}


/* 启动监听服务器
*/
static void _NTInitNTServer(struct config *conf) {
    int listenPort;
    struct configOption *confOpt;
    char tmpstr[TMPSTR_SIZE] = {""};

    confOpt = configGet(conf, "net_server", "port");

    if (NULL != confOpt) {

        if (confOpt->valueLen > TMPSTR_SIZE) {
            trvExit(0, "监听端口太大");
        }

        memcpy(tmpstr, confOpt->value, confOpt->valueLen);
        tmpstr[confOpt->valueLen] = 0;
        listenPort = atoi(tmpstr);
    }
    else listenPort = TRV_NET_LISTEN_PORT;


    if (ERRNO_ERR == NTInitNTServer(listenPort)) {
        trvExit(0, "初始化网络失败");
    }

    /*
    if(aeCreateTimeEvent(g_server.el, 1, serverCron, NULL, NULL) == AE_ERR) {
        trvLogE("Can't create the serverCron time event.");
        exit(1);
    }
    */
    aeSetBeforeSleepProc(g_server.el, beforeSleep);
    aeMain(g_server.el);
    aeDeleteEventLoop(g_server.el);
}


/* 启动星球运行支持
*/
static void _STInitPlanet(struct config *conf) {
    struct configOption *confOpt;
    char tmpstr[ALLOW_PATH_SIZE] = {""};

    confOpt = configGet(conf, "planet", "relative_path");
    if (NULL == confOpt) {
        trvExit(0, "请选择星球文件路径");
    }
    if (confOpt->valueLen > ALLOW_PATH_SIZE) {
        trvExit(0, "星球文件地址太长");
    }
    memcpy(tmpstr, confOpt->value, confOpt->valueLen);
    tmpstr[confOpt->valueLen] = 0;
    snprintf(g_planetdir, ALLOW_PATH_SIZE, "%s/../planet/%s", g_basedir, tmpstr);
    
    STInitPlanet();
}



int main(int argc, char *argv[]) {
    struct config *conf;
    char tmpstr[ALLOW_PATH_SIZE] = {""};

    zmalloc_enable_thread_safeness();

    snprintf(tmpstr, ALLOW_PATH_SIZE, "%s/../", argv[0]);
    if (NULL == realpath(tmpstr, g_basedir)) {
        trvExit(0, "获取当前路径失败");
    }

    conf = initConfig();

    snprintf(tmpstr, ALLOW_PATH_SIZE, "%s/../conf/default.conf", g_basedir);
    configRead(conf, tmpstr);

    if (argc > 1) configRead(conf, argv[1]); /* argv[1] 是配置文件路径 */

    if (NULL == conf->contents) {
        trvExit(0, "请选择配置文件");
    }

    _STInitPlanet(conf);
    //_NTInitNTServer(conf);
    UIinit();

    return 0;
}
