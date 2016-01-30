#ifndef __NETCMD_NETCMD_H
#define __NETCMD_NETCMD_H

#include "net/networking.h"
#include "script/script.h"

typedef struct SVServiceRouter {
    char *key;
    void (*proc)(NTSnode *sn);
    int argc;
} SVServiceRouter;

void SVSubscribeScriptCmd(STScript *script);
void SVPrepare();
void SVScript(NTSnode *sn); //出发脚本服务
void SVMsg(NTSnode *sn);
void SVTest(NTSnode *sn);
void SVClose(NTSnode *sn);

#endif
