#include <pthread.h>

#include "core/util.h"
#include "core/dict.h"
#include "net/networking.h"
#include "netcmd/netcmd.h"

extern struct NTServer g_server;

extern int g_blockCmdFd;
extern pthread_mutex_t g_blockCmdMtx;
extern pthread_cond_t g_blockCmdCond;
extern pthread_mutex_t g_blockNetMtx;
extern pthread_cond_t g_blockNetCond;

dictType commandTableDictType = {
    dictSdsCaseHash,           /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictSdsKeyCaseCompare,     /* key compare */
    dictSdsDestructor,         /* key destructor */
    NULL                       /* val destructor */
};

struct trvCommand trvCommandTable[] = {
    {"planet", planetCommand, 0},
    {"test", testCommand, 0},
    {"msg", msgCommand, 0},
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


/* 某 NTSnode 下的 阻塞模式命令流程
 * TODO，这里限制了进程中，同时只能处理一次阻塞模式的发送命令，考虑使用协程
 *   
 *   .------------------------------------.----------------------------------------------.
 *   |            发起命令的线程          |               网络处理线程                   |
 *   /------------------------------------/----------------------------------------------/
 *   | Step1 准备阻塞(NTPrepareBlockCmd)  |                                              |
 *   |       保证阻塞成功后再执行后续流程 |                                              |
 *   |       网络线程 锁住了命令解析过程  |                                              |
 *   |       阻塞需要等待网络线程执行完   |                                              |
 *   |       关键流程                     |                                              |
 *   /------------------------------------/----------------------------------------------/
 *   | Step2 发送命令(AddReply)           |                                              |
 *   /------------------------------------/----------------------------------------------/
 *   | Step3 阻塞(NTBlockCmd)             |                                              |
 *   /------------------------------------/----------------------------------------------/
 *   |                                    | Step4 解析读入数据完成后，                   |
 *   |                                    |       唤醒线程(NTAwakeBlockCmd)              |
 *   |                                    |       进入睡眠                               |
 *   /------------------------------------/----------------------------------------------/
 *   | Step5 执行代码                     |                                              |
 *   /------------------------------------/----------------------------------------------/
 *   | Step6 执行完成阻塞模式命令函数     |                                              |
 *   |       (NTFinishBlockCmd)           |                                              |
 *   |       唤醒网络处理线程             |                                              |
 *   /------------------------------------/----------------------------------------------/
 *   |                                    | Step7 被NTFinishBlockCmd唤醒                 |
 *   |                                    |       继续网络处理循环                       |
 *   `------------------------------------.----------------------------------------------`
 * 
 */ 

int NTPrepareBlockCmd(NTSnode *sn) {
    if (g_blockCmdFd > 0) {
        trvLogW("已经有 NTSnode 在使用 阻塞模式 发送命令");
        return -1;
    }
    g_blockCmdFd = sn->fd;

    /* Step1 申请阻止网络处理线程 开始解析数据流程 */
    pthread_mutex_lock(&g_blockNetMtx);

    /* Step2 保证命令发送者 线程安全 */
    pthread_mutex_lock(&g_blockCmdMtx);
    return 0;
}

void NTBlockCmd(NTSnode *sn) {
    /* Step3
     * 原子操作
     * Step3-1 发送命令者睡眠，等待数据 
     * Step3-2 允许网络处理线程 开始解析数据流程 
     */
    trvLogI("sleep %d", sn->fd);
    pthread_cond_wait(&g_blockCmdCond, &g_blockNetMtx);
    trvLogI("awake %d", sn->fd);
}

void NTFinishBlockCmd(NTSnode *sn) {
    trvLogI("finshed %d", sn->fd);

    /* Step7 确定网络处理线程正在睡眠 */
    pthread_mutex_lock(&g_blockCmdMtx);
    pthread_mutex_unlock(&g_blockCmdMtx);

    /* Step8 唤醒 网络处理模块 */
    pthread_cond_signal(&g_blockNetCond);
    g_blockCmdFd = 0;
}

void NTAwakeBlockCmd(NTSnode *sn) {
    /* Step4 解锁 g_blockNetMtx，Step3-2将会重新上锁 */
    pthread_mutex_unlock(&g_blockNetMtx);
    /* Step5 唤醒发送命令者 */
    pthread_cond_signal(&g_blockCmdCond);
    trvLogI("wake cmd %d", sn->fd);

    /* Step6
     * 原子操作
     * Step6-1 睡眠，等待发送命令者 唤醒 
     * Step6-2 解锁g_blockCmdMtx，以便 NTFinishBlockCmd 探知网络处理线程已睡眠
     */
    pthread_cond_wait(&g_blockNetCond, &g_blockCmdMtx);

    /* Step9
     * 解锁 Step6-2 中锁定的 g_blockNetMtx
     */
    pthread_mutex_unlock(&g_blockCmdMtx);
}
