#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

#include "lua.h"
#include "core/config.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "net/networking.h"
#include "net/ae.h"
#include "plannet/plannet.h"
#include "sqlite3.h"

#define TMPSTR_SIZE ALLOW_PATH_SIZE

/* 全局变量
 */
struct Server g_server;
/* 绝对路径为 $(traveller)/src
 */
char g_basedir[ALLOW_PATH_SIZE] = {""};
/* 需要加载的星球路径
 */
char g_plannetdir[ALLOW_PATH_SIZE] = {""};
lua_State *g_plannetLuaSt;
sqlite3 *g_plannetDB;




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
static void _initServer(struct config *conf) {
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


    if (ERRNO_ERR == initServer(listenPort)) {
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
static void _initPlannet(struct config *conf) {
    struct configOption *confOpt;
    char tmpstr[TMPSTR_SIZE] = {""};

    confOpt = configGet(conf, "plannet", "relative_path");
    if (NULL == confOpt) {
        trvExit(0, "请选择星球文件路径");
    }
    if (confOpt->valueLen > TMPSTR_SIZE) {
        trvExit(0, "星球文件地址太长");
    }
    memcpy(tmpstr, confOpt->value, confOpt->valueLen);
    tmpstr[confOpt->valueLen] = 0;
    snprintf(g_plannetdir, TMPSTR_SIZE, "%s/../plannet/%s", g_basedir, tmpstr);
    
    initPlannet();
}



int main(int argc, char *argv[]) {
    struct config *conf;
    char tmpstr[TMPSTR_SIZE] = {""};

    zmalloc_enable_thread_safeness();

    snprintf(tmpstr, TMPSTR_SIZE, "%s/../", argv[0]);
    if (NULL == realpath(tmpstr, g_basedir)) {
        trvExit(0, "获取当前路径失败");
    }

    conf = initConfig();

    snprintf(tmpstr, TMPSTR_SIZE, "%s/../conf/default.conf", g_basedir);
    configRead(conf, tmpstr);

    if (argc > 1) configRead(conf, argv[1]); /* argv[1] 是配置文件路径 */

    if (NULL == conf->contents) {
        trvExit(0, "请选择配置文件");
    }

    _initPlannet(conf);
    _initServer(conf);

    return 0;
}
