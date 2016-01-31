#include "script/script.h"

const char* STgetGlobalString(lua_State *L, char *key) {
    const char *ret;
    lua_getglobal(L, key);
    ret = lua_tostring(L, -1);
    return ret;
}

int STLogI(lua_State *L) {
    char *log;
    log = (char *)lua_tostring(L, 1);
    TrvLogI("%s", log);
    return 0;
}

int STLoadView(lua_State *L) {
    sds viewFullPath = sdsempty();
    char * viewPath = (char *)lua_tostring(L, 1);
    viewFullPath = sdscatfmt(viewFullPath, "%s/view/%s.html", STgetGlobalString(L, "g_basedir"), viewPath);

    sds content = file_get_contents(viewFullPath);
    if (0 == content) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, content);
        sdsfree(content);
    }

    return 1;
}
