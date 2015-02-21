#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <pthread.h> 

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
static void* _NTInitNTServer(void *conf) {
    conf = (struct config *)conf;
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
    else listenPort = -1; /* 输入一个不正常的监听端口，意味着不监听 */


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

    return NULL;
}


/* 启动星球运行支持
*/
static void _STInitPlanet(struct config *conf) {
    struct configOption *confOpt;
    char tmpstr[ALLOW_PATH_SIZE] = {""};

    /* 游戏服务端初始化 */
    confOpt = configGet(conf, "planet_server", "relative_path");
    if (confOpt) {
        if (confOpt->valueLen > ALLOW_PATH_SIZE) {
            trvExit(0, "星球文件地址太长");
        }
        memcpy(tmpstr, confOpt->value, confOpt->valueLen);
        tmpstr[confOpt->valueLen] = 0;
        snprintf(g_planetdir, ALLOW_PATH_SIZE, "%s/../planet/%s", g_basedir, tmpstr);
        STInitPlanet();

        _NTInitNTServer(conf);

        return;
    }

    /* 游戏客户端初始化 */
    confOpt = configGet(conf, "planet_client", "relative_path");
    if (confOpt) {
        pthread_t ntid;

        pthread_create(&ntid, NULL, _NTInitNTServer, conf);

        if (confOpt->valueLen > ALLOW_PATH_SIZE) {
            trvExit(0, "星球文件地址太长");
        }
        memcpy(tmpstr, confOpt->value, confOpt->valueLen);
        tmpstr[confOpt->valueLen] = 0;
        snprintf(g_planetdir, ALLOW_PATH_SIZE, "%s/../planet/%s", g_basedir, tmpstr);
        STInitPlanet();
        UIInit();
        return;
    }


    /* 游戏服务端与客户端都初始化失败 */
    trvExit(0, "请选择星球文件路径");
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

    return 0;
}
