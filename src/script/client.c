/* 客户端模式初始化 */
static void PrepareClient() {
    int errno;
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

    lua_pushstring(g_cliLuaSt, g_galaxiesSrvSnode->fdstr);
    lua_setglobal(g_cliLuaSt, "g_serverContenctId");

    char *filepath = (char *)zmalloc(ALLOW_PATH_SIZE);
    memset(filepath, 0, ALLOW_PATH_SIZE);

    snprintf(filepath, ALLOW_PATH_SIZE, "%s/main.lua", g_cliGalaxydir);
    errno = luaL_loadfile(g_cliLuaSt, filepath);
    if (errno) {
        TrvExit(0, "%s", lua_tostring(g_cliLuaSt, -1));
    }
    zfree(filepath);

    //初始化
    errno = lua_pcall(g_cliLuaSt, 0, 0, 0);
    if (errno) {
        TrvExit(0, "%s", lua_tostring(g_cliLuaSt, -1));
    }

    //调用init函数  
    lua_getglobal(g_cliLuaSt, "Init");
    lua_pcall(g_cliLuaSt, 0, 0, 0);
}
