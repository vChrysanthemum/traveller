#include "script/script.h"
#include "g_extern.h"

int STScriptService(STScript *script, NTSnode *sn) {
    int errno;
    char **argv = &(sn->argv[1]);
    int argc = sn->argc - 1;
    char *funcName = *argv;
    int _m;
    lua_State *L = script->L;

    lua_getglobal(L, "ServiceRouter");
    lua_pushstring(L, sn->fdstr);
    lua_pushstring(L, funcName);

    /* 从argv[1]开始，既忽略funcName */
    for (_m = 1; _m < argc; _m++) {
        lua_pushstring(L, argv[_m]);
    }

    errno = lua_pcall(L, argc+1, 0, 0);

    if (0 != errno) {
        TrvLogW("%s", lua_tostring(L, -1));
        return LUA_SERVICE_ERRNO_INNERERR;
    }

    return LUA_SERVICE_ERRNO_OK;
}

int STScriptServiceCallback(NTSnode *sn) {
    int errno;

    lua_getglobal(sn->lua, "ServiceCallbackRouter");
    lua_pushstring(sn->lua, sn->fdstr);
    lua_pushstring(sn->lua, sn->luaCbkUrl);
    if (0 == sdslen(sn->luaCbkArg)) {
        lua_pushnil(sn->lua);
    } else {
        lua_pushstring(sn->lua, sn->luaCbkArg);
    }

    lua_pushnumber(sn->lua, sn->recvType);

    for (int i = 0; i < sn->argvSize; i++) {
        lua_pushstring(sn->lua, sn->argv[i]);
    }

    errno = lua_pcall(sn->lua, 4+sn->argvSize, 0, 0);

    if (0 != errno) {
        TrvLogW("%s", lua_tostring(sn->lua, -1));
        return LUA_SERVICE_ERRNO_INNERERR;
    }

    return LUA_SERVICE_ERRNO_OK;
}
