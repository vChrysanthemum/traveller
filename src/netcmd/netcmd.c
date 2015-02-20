#include "core/util.h"
#include "core/dict.h"
#include "net/networking.h"
#include "netcmd/netcmd.h"

extern struct NTServer g_server;

dictType commandTableDictType = {
    dictSdsCaseHash,           /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictSdsKeyCaseCompare,     /* key compare */
    dictSdsDestructor,         /* key destructor */
    NULL                       /* val destructor */
};

struct trvCommand trvCommandTable[] = {
    {"pla", planetCommand, 0},
    {"test", testCommand, 0},
    {"close", closeCommand, 0}
};

void initNetCmd() {
    int loopJ, tmpsize;

    g_server.commands = dictCreate(&commandTableDictType, NULL);
    tmpsize = sizeof(trvCommandTable) / sizeof(trvCommandTable[0]);
    for (loopJ = 0; loopJ < tmpsize; loopJ++) {
        dictAdd(g_server.commands, sdsnew(trvCommandTable[loopJ].key), trvCommandTable[loopJ].proc);
    }
}
