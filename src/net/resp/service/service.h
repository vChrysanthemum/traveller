#ifndef __NET_PROTOCOL_RESP_SERVICE_H
#define __NET_PROTOCOL_RESP_SERVICE_H

typedef struct ntRespSVServiceRouter_s {
    char *key;
    void (*proc)(ntRespSnode_t *sn);
    int argc;
} ntRespSVServiceRouter_t;

void NTRespSV_SubscribeScriptService(stScript_t *script);
void NTRespSV_Prepare();
void NTRespSV_Script(ntRespSnode_t *sn); //触发脚本服务
void NTRespSV_ScriptCallback(ntRespSnode_t *sn);
void NTRespSV_Msg(ntRespSnode_t *sn);
void NTRespSV_Test(ntRespSnode_t *sn);
void NTRespSV_Close(ntRespSnode_t *sn);

#endif
