#include "core/util.h"
#include "core/zmalloc.h"
#include "net/networking.h"
#include "script/planet.h"
#include "script/db.h"
#include "script/net.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


extern char g_planetdir[];
extern lua_State *g_planetLuaSt;

void STInit() {
    int errno;
    char *filepath = zmalloc(ALLOW_PATH_SIZE);
    memset(filepath, 0, ALLOW_PATH_SIZE);

    STInitDB();

    g_planetLuaSt = luaL_newstate();
    luaL_openlibs(g_planetLuaSt);
    /*
    luaopen_base(g_planetLuaSt);
    luaopen_table(g_planetLuaSt);
    luaopen_string(g_planetLuaSt);
    luaopen_math(g_planetLuaSt);
    */

    snprintf(filepath, ALLOW_PATH_SIZE, "%s/main.lua", g_planetdir);

    lua_pushstring(g_planetLuaSt, g_planetdir);
    lua_setglobal(g_planetLuaSt, "basedir");

    errno = luaL_loadfile(g_planetLuaSt, filepath);
    if (errno) {
        trvExit(0, "%s", lua_tostring(g_planetLuaSt, -1));
    }

    lua_register(g_planetLuaSt, "NTConnectNTSnode", STConnectNTSnode);
    lua_register(g_planetLuaSt, "NTAddReplyString", STAddReplyString);
    lua_register(g_planetLuaSt, "NTAddReplyRawString", STAddReplyRawString);
    lua_register(g_planetLuaSt, "NTAddReplyMultiString", STAddReplyMultiString);
    lua_register(g_planetLuaSt, "DBQuery", STDBQuery);

    //初始化
    errno = lua_pcall(g_planetLuaSt, 0, 0, 0);
    if (errno) {
        trvExit(0, "%s", lua_tostring(g_planetLuaSt, -1));
    }

    //调用init函数  
    lua_getglobal(g_planetLuaSt, "init");
    if (!lua_isfunction(g_planetLuaSt, -1)) {
        trvExit(0, "lua: 找不到init函数");
    }
    lua_pcall(g_planetLuaSt, 0, 0, 0);

    zfree(filepath);

}
