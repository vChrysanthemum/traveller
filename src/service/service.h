#ifndef __NETCMD_NETCMD_H
#define __NETCMD_NETCMD_H

typedef struct svServiceRouter_s {
    char *key;
    void (*proc)(ntSnode_t *sn);
    int argc;
} svServiceRouter_t;

void SV_SubscribeScriptService(stScript_t *script);
void SV_Prepare();
void SV_Script(ntSnode_t *sn); //触发脚本服务
void SV_ScriptCallback(ntSnode_t *sn);
void SV_Msg(ntSnode_t *sn);
void SV_Test(ntSnode_t *sn);
void SV_Close(ntSnode_t *sn);

#endif
