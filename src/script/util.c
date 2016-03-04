#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/util.h"
#include "core/ini.h"
#include "core/extern.h"

#include "script/script.h"

const char* ST_getGlobalString(lua_State *L, char *key) {
    const char *ret;
    lua_getglobal(L, key);
    ret = lua_tostring(L, -1);
    return ret;
}

int ST_LogI(lua_State *L) {
    char *log;
    log = (char *)lua_tostring(L, 1);
    C_LogI("%s", log);
    return 0;
}

int ST_LoadView(lua_State *L) {
    sds viewFullPath = sdsempty();
    char * viewPath = (char *)lua_tostring(L, 1);
    viewFullPath = sdscatfmt(viewFullPath, "%s/view/%s.html", ST_getGlobalString(L, "g_basedir"), viewPath);

    sds content = file_get_contents(viewFullPath);
    if (0 == content) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, content);
        sdsfree(content);
    }

    sdsfree(viewFullPath);

    return 1;
}
