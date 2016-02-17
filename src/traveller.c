#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <locale.h>

#include "core/errors.h"
#include "core/ini.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "core/debug.h"
#include "net/networking.h"
#include "event/event.h"
#include "script/script.h"
#include "ui/ui.h"
#include "ui/map.h"

#include "lua.h"
#include "sqlite3.h"

#include "net/extern.h"

/**
 * 运行流程
 *  Step1. 读取配置文件
 *  Step2. 开启界面
 *  Step3. 开启事件
 *  Step4. 开启3块 etDevice_t g_mainDevice g_netDevice g_fooDevice
 */

/* 全局变量 */
etDevice_t *g_mainDevice;
etDevice_t *g_fooDevice;
etDevice_t *g_netDevice;

char g_basedir[ALLOW_PATH_SIZE] = {""}; /* 绝对路径为 $(traveller)/src */
Log  g_log;
Ini  *g_conf;

list *g_scripts;

int main(int argc, char *argv[]) {
    sds value;
    char tmpstr[ALLOW_PATH_SIZE] = {""};
    int listenPort;

    g_log.dir = 0;
    g_log.f = stderr;
    g_log.fd = STDERR_FILENO;

    setlocale(LC_ALL,"");

    zmalloc_enable_thread_safeness();

    snprintf(tmpstr, ALLOW_PATH_SIZE, "%s/../", argv[0]);
    if (NULL == realpath(tmpstr, g_basedir)) {
        TrvExit(0, "获取当前路径失败");
    }

    g_conf = InitIni();

    snprintf(tmpstr, ALLOW_PATH_SIZE, "%s/../conf/default.conf", g_basedir);
    IniRead(g_conf, tmpstr);

    if (argc > 1) IniRead(g_conf, argv[1]); /* argv[1] 是配置文件路径 */

    if (NULL == g_conf->contents) {
        TrvExit(0, "请选择配置文件");
    }

    value = IniGet(g_conf, "traveller", "log_dir");
    if (0 != value) {
        g_log.dir = stringnewlen(value, sdslen(value));
        g_log.f = fopen(g_log.dir, "a+");
        g_log.fd = fileno(g_log.f);
    }

    value = IniGet(g_conf, "traveller", "listen_port");

    if (0 != value) {
        if (sdslen(value) > ALLOW_PATH_SIZE) {
            TrvExit(0, "监听端口太大");
        }

        listenPort = atoi(value);
    }
    else listenPort = -1; /* 输入一个不正常的监听端口，意味着不监听 */

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    setupSignalHandlers();

    g_mainDevice = ET_NewDevice(0, 0);
    g_fooDevice = ET_NewDevice(0, 0);

    //初始化UI
    if (ERRNO_ERR == UI_Prepare()) {
        TrvExit(0, "初始化UI失败");
    }

    //开启网络
    if (ERRNO_ERR == NT_Prepare(listenPort)) {
        TrvExit(0, "初始化网络失败");
    }
    g_netDevice = ET_NewDevice(aeMainDeviceWrap, nt_el);

    //开启脚本支持
    if (ERRNO_ERR == ST_Prepare()) {
        TrvExit(0, "初始化脚本失败");
    }

    ET_StartDevice(g_mainDevice);
    ET_StartDevice(g_fooDevice);
    ET_StartDevice(g_netDevice);

    //开启界面，并阻塞在 uiLoop
    if (ERRNO_ERR == UI_Init()) {
        TrvExit(0, "初始化UI失败");
    }

    return 0;
}
