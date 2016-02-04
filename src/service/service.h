#ifndef __NETCMD_NETCMD_H
#define __NETCMD_NETCMD_H

#include "net/networking.h"
#include "script/script.h"

typedef struct SVServiceRouter {
    char *key;
    void (*proc)(NTSnode *sn);
    int argc;
} SVServiceRouter;

void SVSubscribeScriptService(STScript *script);
void SVPrepare();
void SVScript(NTSnode *sn); //触发脚本服务
void SVScriptCallback(NTSnode *sn);
void SVMsg(NTSnode *sn);
void SVTest(NTSnode *sn);
void SVClose(NTSnode *sn);

#endif
