#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/util.h"
#include "core/ini.h"
#include "core/zmalloc.h"
#include "core/errors.h"
#include "core/extern.h"

#include "event/event.h"
#include "script/script.h"
#include "net/networking.h"
#include "net/resp/service/service.h"
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

    lua_pushstring(L, script->BaseDir);lua_setglobal(L, "g_basedir");

    lua_newtable(L);lua_setglobal(L, "core");

    lua_getglobal(L, "core");

    lua_pushstring(L, "util");                     lua_newtable(L);
    lua_pushstring(L, "LogI");                     lua_pushcfunction(L, ST_LogI);lua_settable(L, -3);
    lua_pushstring(L, "LoadView");                 lua_pushcfunction(L, ST_LoadView);lua_settable(L, -3);
    lua_settable(L, -3);

    lua_pushstring(L, "net");                      lua_newtable(L);
    lua_pushstring(L, "resp");                     lua_newtable(L);
    lua_pushstring(L, "ConnectSnode");             lua_pushcfunction(L, STNTResp_ConnectSnode);lua_settable(L, -3);
    lua_pushstring(L, "ScriptServiceRequest");     lua_pushcfunction(L, STNTResp_ScriptServiceRequest);lua_settable(L, -3);
    lua_pushstring(L, "ScriptServiceResponse");    lua_pushcfunction(L, STNTResp_ScriptServiceResponse);lua_settable(L, -3);
    lua_pushstring(L, "AddReplyString");           lua_pushcfunction(L, STNTResp_AddReplyString);lua_settable(L, -3);
    lua_pushstring(L, "AddReplyRawString");        lua_pushcfunction(L, STNTResp_AddReplyRawString);lua_settable(L, -3);
    lua_pushstring(L, "AddReplyMultiString");      lua_pushcfunction(L, STNTResp_AddReplyMultiString);lua_settable(L, -3);
    lua_settable(L, -3);
    lua_settable(L, -3);

    lua_pushstring(L, "db");                       lua_newtable(L);
    lua_pushstring(L, "Connect");                  lua_pushcfunction(L, STDB_Connect);lua_settable(L, -3);
    lua_pushstring(L, "Close");                    lua_pushcfunction(L, STDB_Close);lua_settable(L, -3);
    lua_pushstring(L, "Query");                    lua_pushcfunction(L, STDB_Query);lua_settable(L, -3);
    lua_settable(L, -3);

    lua_pushstring(L, "ui");                       lua_newtable(L);
    lua_pushstring(L, "LoadPage");                 lua_pushcfunction(L, STUI_LoadPage);lua_settable(L, -3);
    lua_settable(L, -3);

    lua_settable(L, -3);

    int errno;

    sds filepath = sdscatprintf(sdsempty(), "%s/main.lua", script->BaseDir);
    errno = luaL_loadfile(L, filepath);
    if (errno) {
        C_UtilExit(0, "%s", lua_tostring(L, -1));
    }
    sdsfree(filepath);

    //初始化
    errno = lua_pcall(L, 0, 0, 0);
    if (errno) {
        C_UtilExit(0, "%s", lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    //调用init函数  
    lua_getglobal(L, "Init");

    IniOption *iniOption;
    dictEntry *de;
    dictIterator *di = dictGetIterator(script->IniSection->Options);
    lua_newtable(L);
    while ((de = dictNext(di)) != 0) {
        iniOption = (IniOption*)dictGetVal(de);

        lua_pushstring(L, iniOption->Key);
        lua_pushstring(L, iniOption->Value);
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
    
    script->IniSection = iniSection;

    sds value;
    sds dir = sdsempty();

    value = IniGet(g_conf, script->IniSection->Key, "basedir");
    if (0 == value) {
        C_UtilExit(0, "%s 缺失脚本路径", script->IniSection->Key);
    }
    dir = sdscatprintf(dir, "%s/%s", g_scriptBaseDir, value);
    script->BaseDir = sdsnew(dir);

    ST_InitScriptLua(script);

    sdsfree(dir);
    return script;
}

void ST_FreeScript(void *_script) {
    stScript_t *script = _script;
    sdsfree(script->BaseDir);
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
    dictIterator *di = dictGetIterator(g_conf->Sections);
    while ((de = dictNext(di)) != 0) {
        section = (IniSection*)dictGetVal(de);
        if (sdslen(section->Key) < headerLen) {
            continue;
        }

        if (0 != memcmp(section->Key, header, headerLen)) {
            continue;
        }

        script = ST_NewScript(section);

        g_scripts = listAddNodeTail(g_scripts, script);

        value = IniGet(g_conf, section->Key, "is_subscribe_net");
        if (0 != value && 0 == sdscmpstr(value, "1")) {
            NTRespSV_SubscribeScriptService(script);
        }
    }
    dictReleaseIterator(di);

    return ERRNO_OK;
}
