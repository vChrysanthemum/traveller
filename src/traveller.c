#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <locale.h>
#include <pthread.h>

#include "core/errors.h"
#include "core/config.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "core/debug.h"
#include "net/networking.h"
#include "event/ae.h"
#include "script/script.h"
#include "script/galaxies.h"
#include "ui/ui.h"
#include "ui/map.h"

#include "lua.h"
#include "sqlite3.h"

/**
 * 运行流程
 *  Step1. 读取配置文件
 *  Step2. 开启界面
 *  Step3. 开启事件
 *  Step4. 事件不断循环，并传递消息给界面管理器，界面管理器更新界面
 *  Step5. 界面响应用户，并传递消息给事件管理器
 *  +------------+        +------------+
 *  |            |  ===>  |            |
 *  |  界面线程  |        |  事件线程  |
 *  |            |  <===  |            |
 *  +------------+        +------------+
 */

/* 全局变量 */
aeEventLoop *g_el;
NTServer g_server;
char g_basedir[ALLOW_PATH_SIZE] = {""}; /* 绝对路径为 $(traveller)/src */
char *g_logdir;
FILE* g_logF;
int g_logFInt;
config *g_conf;

/* UI部分 */

/* 服务端模式所需变量 */
char g_srvGalaxydir[ALLOW_PATH_SIZE] = {""}; /* 需要加载的星系路径 */
lua_State *g_srvLuaSt;
sqlite3 *g_srvDB;

/* 客户端模式所需变量 */
char g_cliGalaxydir[ALLOW_PATH_SIZE] = {""}; /* 需要加载的星系路径 */
lua_State *g_cliLuaSt;
sqlite3 *g_cliDB;
NTSnode *g_galaxiesSrvSnode; /* 星系服务端连接 */

/*
static int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData) {
    g_server.unixtime = time(NULL);
    TrvLogI("serverCron");
    return 0;
}
*/

//循环处理事件
static void* eventLoop(void* _) {
    /*
    if(aeCreateTimeEvent(g_el, 1, serverCron, NULL, NULL) == AE_ERR) {
        TrvLogE("Can't create the serverCron time event.");
        exit(1);
    }
    */

    //aeSetBeforeSleepProc(g_el, beforeSleep);

    aeMain(g_el);
    aeDeleteEventLoop(g_el);

    return NULL;
}

int main(int argc, char *argv[]) {
    struct configOption *confOpt;
    char tmpstr[ALLOW_PATH_SIZE] = {""};
    int listenPort;

    g_logdir = NULL;
    g_logF = stderr;
    g_logFInt = STDERR_FILENO;

    setlocale(LC_ALL,"");

    zmalloc_enable_thread_safeness();

    snprintf(tmpstr, ALLOW_PATH_SIZE, "%s/../", argv[0]);
    if (NULL == realpath(tmpstr, g_basedir)) {
        TrvExit(0, "获取当前路径失败");
    }

    g_conf = initConfig();

    snprintf(tmpstr, ALLOW_PATH_SIZE, "%s/../conf/default.conf", g_basedir);
    configRead(g_conf, tmpstr);

    if (argc > 1) configRead(g_conf, argv[1]); /* argv[1] 是配置文件路径 */

    if (NULL == g_conf->contents) {
        TrvExit(0, "请选择配置文件");
    }

    confOpt = configGet(g_conf, "traveller", "log_dir");
    if (confOpt) {
        g_logdir = (char *)zmalloc(confOpt->valueLen+1);
        snprintf(g_logdir, ALLOW_PATH_SIZE, "%.*s", confOpt->valueLen, confOpt->value);
        g_logF = fopen(g_logdir, "a+");
        g_logFInt = fileno(g_logF);
    }

    confOpt = configGet(g_conf, "net_server", "port");

    if (NULL != confOpt) {

        if (confOpt->valueLen > ALLOW_PATH_SIZE) {
            TrvExit(0, "监听端口太大");
        }

        confOptToStr(confOpt, tmpstr);
        listenPort = atoi(tmpstr);
    }
    else listenPort = -1; /* 输入一个不正常的监听端口，意味着不监听 */

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    setupSignalHandlers();

    //开启事件
    g_el = aeCreateEventLoop(1024*1024);

    //开启网络
    if (ERRNO_ERR == NTInit(listenPort)) {
        TrvExit(0, "初始化网络失败");
    }

    //开启脚本支持
    if (ERRNO_ERR == STInit()) {
        TrvExit(0, "初始化UI失败");
    }

    /* 循环 */
    pthread_t ntid;
    pthread_create(&ntid, NULL, eventLoop, NULL);

    //开启界面，并阻塞在 uiLoop
    if (ERRNO_ERR == UIInit()) {
        TrvExit(0, "初始化UI失败");
    }

    return 0;
}
