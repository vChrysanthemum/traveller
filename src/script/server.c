/* 服务端模式初始化 */
static void PrepareServer() {
    int errno;
    STInitLua(&g_srvLuaSt, g_srvGalaxydir);

    char *filepath = (char *)zmalloc(ALLOW_PATH_SIZE);
    memset(filepath, 0, ALLOW_PATH_SIZE);

    snprintf(filepath, ALLOW_PATH_SIZE, "%s/main.lua", g_srvGalaxydir);
    errno = luaL_loadfile(g_srvLuaSt, filepath);
    if (errno) {
        TrvExit(0, "%s", lua_tostring(g_srvLuaSt, -1));
    }

    memset(filepath, 0, ALLOW_PATH_SIZE);

    snprintf(filepath, ALLOW_PATH_SIZE, "%s/sqlite.db", g_srvGalaxydir);
    g_srvDB = STinitDB(filepath);

    zfree(filepath);

    //初始化
    errno = lua_pcall(g_srvLuaSt, 0, 0, 0);
    if (errno) {
        TrvExit(0, "%s", lua_tostring(g_srvLuaSt, -1));
    }

    //调用init函数  
    lua_getglobal(g_srvLuaSt, "Init");
    lua_pcall(g_srvLuaSt, 0, 0, 0);
}
