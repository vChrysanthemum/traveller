#ifndef __NETCMD_NETCMD_H
#define __NETCMD_NETCMD_H

#include "net/networking.h"

typedef struct SVServiceRouter {
    char *key;
    void (*proc)(NTSnode *sn);
    int argc;
} SVServiceRouter;

void SVInit();

void SVGalaxies(NTSnode *sn); // 发送给星系的命令 
void SVMsg(NTSnode *sn);
void SVTest(NTSnode *sn);
void SVClose(NTSnode *sn);

#endif
