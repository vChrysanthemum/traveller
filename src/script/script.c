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

    IniOption *iniOption;
    dictEntry *de;
    dictIterator *di = dictGetIterator(script->iniSection->options);
    lua_newtable(L);
    while ((de = dictNext(di)) != NULL) {
        iniOption = (IniOption*)dictGetVal(de);

        lua_pushstring(L, iniOption->key);
        lua_pushstring(L, iniOption->value);
        lua_settable(L, -3);
    }
    dictReleaseIterator(di);

    errno = lua_pcall(L, 1, 0, 0);
    STAssertLuaPCallSuccess(L, errno);

    script->L = L;
}


STScript* STNewScript(IniSection *iniSection) {
    STScript *script = (STScript*)zmalloc(sizeof(STScript));
    memset(script, 0, sizeof(STScript));
    
    script->iniSection = iniSection;

    sds value;
    sds dir = sdsempty();

    value = IniGet(g_conf, script->iniSection->key, "basedir");
    if (0 == value) {
        TrvExit(0, "%s 缺失脚本路径", script->iniSection->key);
    }
    dir = sdscatprintf(dir, "%s/../galaxies/%s", g_basedir, value);
    script->basedir = sdsnew(dir);

    STInitScriptLua(script);

    sdsfree(dir);
    return script;
}

void STFreeScript(void *_script) {
    STScript *script = _script;
    sdsfree(script->basedir);
    lua_close(script->L);
    zfree(script);
}

/* 基础部分初始化 */
int STPrepare() {
    g_scripts = listCreate();
    g_scripts->free = STFreeScript;

    sds value;

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

        script = STNewScript(section);

        g_scripts = listAddNodeTail(g_scripts, script);

        value = IniGet(g_conf, section->key, "is_subscribe_net");
        if (0 != value && 0 == sdscmpstr(value, "1")) {
            SVSubscribeScriptCmd(script);
        }
    }
    dictReleaseIterator(di);

    return ERRNO_OK;
}
