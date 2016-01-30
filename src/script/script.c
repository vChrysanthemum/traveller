#include "core/util.h"
#include "core/ini.h"
#include "core/zmalloc.h"
#include "script/script.h"
#include "net/networking.h"
#include "service/service.h"
#include "ui/ui.h"
#include "g_extern.h"

static void STInitScriptLua(STScript *script) {
    lua_State *L;
    L = luaL_newstate();
    luaL_openlibs(L);
    /*
    luaopen_base(L);
    */
    luaopen_table(L);
    luaopen_string(L);
    luaopen_math(L);

    lua_pushstring(L, script->basedir);lua_setglobal(L, "g_basedir");

    lua_register(L, "LogI",                    STLogI);
    lua_register(L, "LoadView",                STLoadView);
    lua_register(L, "NTConnectNTSnode",        STConnectNTSnode);
    lua_register(L, "NTAddReplyString",        STAddReplyString);
    lua_register(L, "NTAddReplyRawString",     STAddReplyRawString);
    lua_register(L, "NTAddReplyMultiString",   STAddReplyMultiString);
    lua_register(L, "DBConnect",               STConnectDB);
    lua_register(L, "DBClose",                 STCloseDB);
    lua_register(L, "DBQuery",                 STDBQuery);

    int errno;

    sds filepath = sdscatprintf(sdsempty(), "%s/main.lua", script->basedir);
    errno = luaL_loadfile(L, filepath);
    if (errno) {
        TrvExit(0, "%s", lua_tostring(L, -1));
    }
    sdsfree(filepath);

    //初始化
    errno = lua_pcall(L, 0, 0, 0);
    if (errno) {
        TrvExit(0, "%s", lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    //调用init函数  
    lua_getglobal(L, "Init");
    for (int i = 0; i < g_conf->contentsCount; i++) {
        lua_pushstring(L, g_conf->contents[i]);
    }
    errno = lua_pcall(L, g_conf->contentsCount, 0, 0);
    STAssertLuaPCallSuccess(L, errno);

    script->L = L;
}


STScript* STNewScript(char *basedir) {
    STScript *script = (STScript*)zmalloc(sizeof(STScript));
    memset(script, 0, sizeof(STScript));
    script->basedir = sdsnew(basedir);
    STInitScriptLua(script);
    return script;
}

void STFreeScript(void *_script) {
    STScript *script = _script;
    sdsfree(script->basedir);
    lua_close(script->L);
    zfree(script);
}

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
    lua_getglobal(sn->lua, "SrvCallbackRouter");
    lua_pushstring(sn->lua, sn->fdstr);
    lua_pushstring(sn->lua, sn->luaCbkUrl);
    if (0 == sdslen(sn->luaCbkArg)) {
        lua_pushnil(sn->lua);
    } else {
        lua_pushstring(sn->lua, sn->luaCbkArg);
    }

    lua_pushnumber(sn->lua, sn->recvType);
    int _m;
    for (_m = 0; _m < sn->argvSize; _m++) {
        lua_pushstring(sn->lua, sn->argv[_m]);
    }

    return lua_pcall(sn->lua, 3 + 1+sn->argvSize, 0, 0);
}

/* 基础部分初始化 */
int STPrepare() {
    g_scripts = listCreate();
    g_scripts->free = STFreeScript;

    sds value;
    sds dir = sdsempty();

    char *header = "script:";
    int headerLen = strlen(header);

    STScript *script;

    IniSection *section;
    dictEntry *de;
    dictIterator *di = dictGetIterator(g_conf->sections);
    while ((de = dictNext(di)) != NULL) {
        section = (IniSection*)dictGetVal(de);
        if (sdslen(section->key) < headerLen) {
            continue;
        }

        if (0 != memcmp(section->key, header, headerLen)) {
            continue;
        }

        value = IniGet(g_conf, section->key, "basedir");
        if (0 == value) {
            TrvExit(0, "%s 缺失脚本路径", section->key);
        }

        sdsclear(dir); 
        dir = sdscatprintf(dir, "%s/../galaxies/%s", g_basedir, value);

        script = STNewScript(dir);

        g_scripts = listAddNodeTail(g_scripts, script);

        value = IniGet(g_conf, section->key, "is_subscribe_net");
        if (0 != value && 0 == sdscmpstr(value, "1")) {
            SVSubscribeScriptCmd(script);
        }
    }
    dictReleaseIterator(di);

    sdsfree(dir);

    return ERRNO_OK;
}
