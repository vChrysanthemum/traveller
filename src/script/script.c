#include "core/util.h"
#include "core/ini.h"
#include "core/zmalloc.h"
#include "script/script.h"
#include "net/networking.h"
#include "service/service.h"
#include "ui/ui.h"
#include "g_extern.h"

etDevice_t *st_device;

static void ST_InitScriptLua(stScript_t *script) {
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

    lua_register(L, "LogI",                     ST_LogI);
    lua_register(L, "LoadView",                 ST_LoadView);
    lua_register(L, "NT_ConnectSnode",          STNT_ConnectSnode);
    lua_register(L, "NT_ScriptServiceRequest",  STNT_ScriptServiceRequest);
    lua_register(L, "NT_ScriptServiceResponse", STNT_ScriptServiceResponse);
    lua_register(L, "NT_AddReplyString",        STNT_AddReplyString);
    lua_register(L, "NT_AddReplyRawString",     STNT_AddReplyRawString);
    lua_register(L, "NT_AddReplyMultiString",   STNT_AddReplyMultiString);
    lua_register(L, "DB_Connect",               STDB_Connect);
    lua_register(L, "DB_Close",                 STDB_Close);
    lua_register(L, "DB_Query",                 STDB_Query);
    lua_register(L, "UI_LoadPage",              STUI_LoadPage);

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
    ST_AssertLuaPCallSuccess(L, errno);

    script->L = L;
}


stScript_t* ST_NewScript(IniSection *iniSection) {
    stScript_t *script = (stScript_t*)zmalloc(sizeof(stScript_t));
    memset(script, 0, sizeof(stScript_t));
    
    script->iniSection = iniSection;

    sds value;
    sds dir = sdsempty();

    value = IniGet(g_conf, script->iniSection->key, "basedir");
    if (0 == value) {
        TrvExit(0, "%s 缺失脚本路径", script->iniSection->key);
    }
    dir = sdscatprintf(dir, "%s/../galaxies/%s", g_basedir, value);
    script->basedir = sdsnew(dir);

    ST_InitScriptLua(script);

    sdsfree(dir);
    return script;
}

void ST_FreeScript(void *_script) {
    stScript_t *script = _script;
    sdsfree(script->basedir);
    lua_close(script->L);
    zfree(script);
}

/* 基础部分初始化 */
int ST_Prepare() {
    st_device = g_netDevice;
    g_scripts = listCreate();
    g_scripts->free = ST_FreeScript;

    sds value;

    char *header = "script:";
    int headerLen = strlen(header);

    stScript_t *script;

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

        script = ST_NewScript(section);

        g_scripts = listAddNodeTail(g_scripts, script);

        value = IniGet(g_conf, section->key, "is_subscribe_net");
        if (0 != value && 0 == sdscmpstr(value, "1")) {
            SV_SubscribeScriptService(script);
        }
    }
    dictReleaseIterator(di);

    return ERRNO_OK;
}
