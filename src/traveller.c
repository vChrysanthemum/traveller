#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <locale.h>

#include "lua.h"
#include "sqlite3.h"

#include "core/sds.h"
#include "core/dict.h"
#include "core/adlist.h"
#include "core/errors.h"
#include "core/ini.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "core/debug.h"
#include "core/extern.h"

#include "net/networking.h"
#include "event/event.h"
#include "script/script.h"
#include "ui/ui.h"

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

char g_basedir[ALLOW_PATH_SIZE] = {""};
char *g_scriptBaseDir;
Ini  *g_conf;

list *g_scripts;

int main(int argc, char *argv[]) {
    sds value;
    char tmpstr[ALLOW_PATH_SIZE] = {""};
    int listenPort;

    c_log.dir = 0;
    c_log.f = stderr;
    c_log.fd = STDERR_FILENO;

    setlocale(LC_ALL,"");

    zmalloc_enable_thread_safeness();

    snprintf(tmpstr, ALLOW_PATH_SIZE, "%s/../", argv[0]);
    if (0 == realpath(tmpstr, g_basedir)) {
        C_UtilExit(0, "获取当前路径失败");
    }

    g_conf = InitIni();

    if (argc > 1) IniRead(g_conf, argv[1]); /* argv[1] 是配置文件路径 */

    if (0 == g_conf->Contents) {
        C_UtilExit(0, "请选择配置文件");
    }

    value = IniGet(g_conf, "traveller", "script_basedir");
    if (0 != value) {
        g_scriptBaseDir = stringnewlen(value, sdslen(value));
    }

    value = IniGet(g_conf, "traveller", "log_path");
    if (0 != value) {
        c_log.dir = stringnewlen(value, sdslen(value));
        c_log.f = fopen(c_log.dir, "a+");
        c_log.fd = fileno(c_log.f);
    }

    value = IniGet(g_conf, "traveller", "listen_port");

    if (0 != value) {
        if (sdslen(value) > ALLOW_PATH_SIZE) {
            C_UtilExit(0, "监听端口太大");
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
        C_UtilExit(0, "初始化UI失败");
    }

    //开启RESP网络
    if (ERRNO_ERR == NTResp_Prepare(listenPort)) {
        C_UtilExit(0, "初始化网络失败");
    }
    g_netDevice = ET_NewDevice(aeMainDeviceWrap, nt_el);

    //开启脚本支持
    if (ERRNO_ERR == ST_Prepare()) {
        C_UtilExit(0, "初始化脚本失败");
    }

    ET_StartDevice(g_mainDevice);
    ET_StartDevice(g_fooDevice);
    ET_StartDevice(g_netDevice);

    //开启界面，并阻塞在 uiLoop
    if (ERRNO_ERR == UI_Init()) {
        C_UtilExit(0, "初始化UI失败");
    }

    return 0;
}
