#include "core/util.h"
#include "core/dict.h"
#include "net/networking.h"
#include "netcmd/netcmd.h"

extern NTServer g_server;

dictType commandTableDictType = {
    dictSdsCaseHash,           /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictSdsKeyCaseCompare,     /* key compare */
    dictSdsDestructor,         /* key destructor */
    NULL                       /* val destructor */
};

struct TrvCommand TrvCommandTable[] = {
    {"galaxies", galaxiesCommand, 0},
    {"test", testCommand, 0},
    {"msg", msgCommand, 0},
    {"close", closeCommand, 0}
};

void initNetCmd() {
    int loopJ, tmpsize;

    g_server.commands = dictCreate(&commandTableDictType, NULL);
    tmpsize = sizeof(TrvCommandTable) / sizeof(TrvCommandTable[0]);
    for (loopJ = 0; loopJ < tmpsize; loopJ++) {
        dictAdd(g_server.commands, sdsnew(TrvCommandTable[loopJ].key), TrvCommandTable[loopJ].proc);
    }
}
