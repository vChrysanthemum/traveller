#include "script/script.h"
#include "g_extern.h"

int STScriptService(STScript *script, NTSnode *sn) {
    int errno;
    char **argv = &(sn->argv[1]);
    int argc = sn->argc - 1;
    char *funcName = *argv;
    lua_State *L = script->L;

    lua_getglobal(L, "ServiceRouter");
    lua_pushstring(L, sn->fdstr);
    lua_pushstring(L, funcName);

    lua_newtable(L);

    /* 从argv[1]开始，既忽略funcName */
    for (int i = 1; i < argc; i += 2) {
        lua_pushstring(L, argv[i]);
        lua_pushstring(L, argv[i+1]);
        lua_settable(L, -3);
    }

    errno = lua_pcall(L, 3, 0, 0);

    if (0 != errno) {
        TrvLogW("%s", lua_tostring(L, -1));
        return LUA_SERVICE_ERRNO_INNERERR;
    }

    return LUA_SERVICE_ERRNO_OK;
}

int STScriptServiceCallback(NTSnode *sn) {
    int errno;
    lua_State *L = sn->lua;

    lua_getglobal(L, "ServiceCallbackRouter");
    lua_pushstring(L, sn->fdstr);
    lua_pushstring(L, sn->luaCbkUrl);
    if (0 == sdslen(sn->luaCbkArg)) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, sn->luaCbkArg);
    }

    lua_pushnumber(L, sn->recvType);

    lua_newtable(L);

    for (int i = 0; i < sn->argvSize; i++) {
        lua_pushstring(L, sn->argv[i]);
        lua_pushstring(L, sn->argv[i+1]);
        lua_settable(L, -3);
    }

    errno = lua_pcall(L, 5, 0, 0);

    if (0 != errno) {
        TrvLogW("%s", lua_tostring(L, -1));
        return LUA_SERVICE_ERRNO_INNERERR;
    }

    return LUA_SERVICE_ERRNO_OK;
}
