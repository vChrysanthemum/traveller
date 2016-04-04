#ifndef __SCRIPT_SCRIPT_H
#define __SCRIPT_SCRIPT_H

#define SCRIPT_SERVICE_ERRNO_OK         0
#define SCRIPT_SERVICE_ERRNO_INNERERR   502

#define ST_AssertLuaPCallSuccess(L, errno) do {\
    if (0 != errno) {\
        C_UtilExit(errno, "%s %d", lua_tostring(L, -1), errno);\
    }\
} while(0);

typedef struct stScript_s {
    int        IsSubscribeNet;
    sds        BaseDir;
    IniSection *IniSection;
    lua_State  *L;
} stScript_t;

stScript_t* ST_NewScript(IniSection *iniSection);
void ST_FreeScript(void *script);

int ST_Prepare();

#define ST_AddReplyHeader(L) \
    const char *fdstr;\
    int argc;\
    ntRespSnode_t *sn;\
\
    argc = lua_gettop(L);\
    if (argc < 3) {\
        lua_pushnumber(L, -1);\
        return 1;\
    }\
\
    fdstr = lua_tostring(L, 1);\
    sn = NTResp_GetSnodeByFDS(fdstr);\
    if (0 == sn) {\
        C_UtilLogI("找不到链接: %s", fdstr);\
        lua_pushnumber(L, -2);\
        return 1;\
    }\
\

const char* ST_getGlobalString(lua_State *L, char *key);
int ST_LogI(lua_State *L);
int ST_LoadView(lua_State *L);
int STNTResp_ScriptServiceRequest(lua_State *L);
int STNTResp_ScriptServiceResponse(lua_State *L);
int STNTResp_AddReplyString(lua_State *L);
int STNTResp_AddReplyMultiString(lua_State *L);
int STNTResp_AddReplyRawString(lua_State *L);
int STNTResp_ConnectSnode(lua_State *L);
int STDB_Connect(lua_State *L);
int STDB_Close(lua_State *L);
int STDB_Query(lua_State *L);
int STUI_LoadPage(lua_State *L);

#endif
