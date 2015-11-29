#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <locale.h>
#include <pthread.h>

#include "core/config.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "core/debug.h"
#include "net/networking.h"
#include "net/ae.h"
#include "script/script.h"
#include "script/galaxies.h"
#include "ui/ui.h"
#include "ui/map.h"

#include "lua.h"
#include "sqlite3.h"

#define TMPSTR_SIZE ALLOW_PATH_SIZE

/* 全局变量 */
struct NTServer g_server;
char g_basedir[ALLOW_PATH_SIZE] = {""}; /* 绝对路径为 $(zeus)/src */
char *g_logdir;
FILE* g_logF;
int g_logFInt;
struct config *g_conf;
void *g_tmpPtr;

/* UI部分 */
UIWin *g_rootUIWin;
UICursor g_cursor;
UIMap *g_curUIMap;

/* 主线程同步 */
pthread_mutex_t g_rootThreadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_rootThreadCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t g_logMutex = PTHREAD_MUTEX_INITIALIZER;


/* 协助阻塞模式发送命令 同步 */
/* 命令发送者使用 */
int g_blockCmdFd;   /* 标识哪一 NTSnode 正在使用阻塞模式发送命令 */
pthread_mutex_t g_blockCmdMtx = PTHREAD_MUTEX_INITIALIZER;

/* 保证数据解析一致性 */
pthread_mutex_t g_blockNetWMtx = PTHREAD_MUTEX_INITIALIZER;


/* 服务端模式所需变量 */
char g_srvGalaxydir[ALLOW_PATH_SIZE] = {""}; /* 需要加载的星系路径 */
lua_State *g_srvLuaSt;
sqlite3 *g_srvDB;

/* 客户端模式所需变量 */
char g_cliGalaxydir[ALLOW_PATH_SIZE] = {""}; /* 需要加载的星系路径 */
lua_State *g_cliLuaSt;
sqlite3 *g_cliDB;
NTSnode *g_galaxiesSrvSnode; /* 星系服务端连接 */




void beforeSleep(struct aeEventLoop *eventLoop) {
    NOTUSED(eventLoop);
}

int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData) {
    g_server.unixtime = time(NULL);
    ZeusLogI("serverCron");
    return 0;
}


/* 启动网络
*/
static void* _NTInit(void* _v) {
    pthread_mutex_lock(&g_rootThreadMutex);

    int listenPort;
    struct configOption *confOpt;
    char tmpstr[TMPSTR_SIZE] = {""};

    confOpt = configGet(g_conf, "net_server", "port");

    if (NULL != confOpt) {

        if (confOpt->valueLen > TMPSTR_SIZE) {
            ZeusExit(0, "监听端口太大");
        }

        memcpy(tmpstr, confOpt->value, confOpt->valueLen);
        tmpstr[confOpt->valueLen] = 0;
        listenPort = atoi(tmpstr);
    }
    else listenPort = -1; /* 输入一个不正常的监听端口，意味着不监听 */


    if (ERRNO_ERR == NTInit(listenPort)) {
        ZeusExit(0, "初始化网络失败");
    }

    /*
    if(aeCreateTimeEvent(g_server.el, 1, serverCron, NULL, NULL) == AE_ERR) {
        ZeusLogE("Can't create the serverCron time event.");
        exit(1);
    }
    */


    aeSetBeforeSleepProc(g_server.el, beforeSleep);

    pthread_mutex_unlock(&g_rootThreadMutex);

    pthread_cond_signal(&g_rootThreadCond);

    aeMain(g_server.el);
    aeDeleteEventLoop(g_server.el);

    return NULL;
}


/* 启动星系运行支持
*/
static void* _STInitGalaxy(void* _v) {
    struct configOption *confOpt;
    char tmpstr[ALLOW_PATH_SIZE] = {""};


    /* 游戏服务端初始化 */
    confOpt = configGet(g_conf, "galaxies_server", "relative_path");
    if (confOpt) {

        if (confOpt->valueLen > ALLOW_PATH_SIZE) {
            ZeusExit(0, "星系文件地址太长");
        }
        confOptToStr(confOpt, tmpstr);
        snprintf(g_srvGalaxydir, ALLOW_PATH_SIZE, "%s/../galaxies/%s", g_basedir, tmpstr);
        STServerInit();
    }


    /* 游戏客户端初始化 */
    confOpt = configGet(g_conf, "galaxies_client", "relative_path");
    if (confOpt) {

        if (confOpt->valueLen > ALLOW_PATH_SIZE) {
            ZeusExit(0, "星系文件地址太长");
        }
        confOptToStr(confOpt, tmpstr);
        snprintf(g_cliGalaxydir, ALLOW_PATH_SIZE, "%s/../galaxies/%s", g_basedir, tmpstr);
        STClientInit();
    }

    return NULL;
}



int main(int argc, char *argv[]) {
    struct configOption *confOpt;
    char tmpstr[ALLOW_PATH_SIZE] = {""};
    pthread_t ntid;

    g_logdir = NULL;
    g_logF = stderr;
    g_logFInt = STDERR_FILENO;

    setlocale(LC_ALL,"");

    zmalloc_enable_thread_safeness();

    snprintf(tmpstr, ALLOW_PATH_SIZE, "%s/../", argv[0]);
    if (NULL == realpath(tmpstr, g_basedir)) {
        ZeusExit(0, "获取当前路径失败");
    }

    g_conf = initConfig();

    snprintf(tmpstr, ALLOW_PATH_SIZE, "%s/../conf/default.conf", g_basedir);
    configRead(g_conf, tmpstr);

    if (argc > 1) configRead(g_conf, argv[1]); /* argv[1] 是配置文件路径 */

    if (NULL == g_conf->contents) {
        ZeusExit(0, "请选择配置文件");
    }

    confOpt = configGet(g_conf, "log", "dir");
    if (confOpt) {
        g_logdir = (char *)zmalloc(confOpt->valueLen+1);
        memcpy(g_logdir, confOpt->value, confOpt->valueLen);
        g_logdir[confOpt->valueLen] = 0;
        g_logF = fopen(g_logdir, "a+");
        g_logFInt = fileno(g_logF);
    }

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    setupSignalHandlers();


    /**
     * 主线程睡眠，等待网络就绪
     * 单独开一个线程处理网路
     */
    pthread_mutex_lock(&g_rootThreadMutex);
    pthread_create(&ntid, NULL, _NTInit, NULL);
    pthread_cond_wait(&g_rootThreadCond, &g_rootThreadMutex);
    pthread_mutex_unlock(&g_rootThreadMutex);


    _STInitGalaxy(NULL);


    /* 主线程睡眠，避免退出进程 */
    pthread_cond_wait(&g_rootThreadCond, &g_rootThreadMutex);
    pthread_mutex_unlock(&g_rootThreadMutex);

    return 0;
}
