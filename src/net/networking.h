#ifndef __NET_NETWORKING_H
#define __NET_NETWORKING_H

#include "core/adlist.h"
#include "core/sds.h"
#include "core/dict.h"
#include "net/ae.h"
#include "net/anet.h"

/* networking，分装了所有的网络操作
 */

#define TRV_NET_LISTEN_PORT 1092
#define TRV_NET_TCP_BACKLOG 1024
#define TRV_NET_MAX_SNODE 1024 * 24
#define TRV_NET_TCPKEEPALIVE 0 
#define TRV_NET_MAX_ACCEPTS_PER_CALL 1000
#define TRV_NET_IOBUF_LEN (1024)  /* Generic I/O buffer size */

/* 基于RESP通信协议，不过有点不一样
 * 通信Snode接受信息各个状态如下：
 */
#define SNODE_RECV_STAT_ACCEPT   0  /* 客户端首次连接，socket accept后 */
#define SNODE_RECV_STAT_PREPARE  1  /* 准备就绪，等待数据中 */
#define SNODE_RECV_STAT_PARSING  3  /* 解析数据中 */
#define SNODE_RECV_STAT_PARSED   4  /* 数据完成 */
#define SNODE_RECV_STAT_EXCUTING 5  /* 正在执行中 */
#define SNODE_RECV_STAT_EXCUTED  6  /* 命令执行完成 */
#define setSnodeExcuteCommandFinished(sn) sn->recv_stat = SNODE_RECV_STAT_EXCUTED;

/* 各个类型解析状态
 */
#define SNODE_RECV_STAT_PARSING_START       0 
#define SNODE_RECV_STAT_PARSING_ARGC        1
#define SNODE_RECV_STAT_PARSING_ARGV_NUM    2
#define SNODE_RECV_STAT_PARSING_ARGV_VALUE  3
#define SNODE_RECV_STAT_PARSING_FINISHED    4


/* 对于traveller的网络库来说，每个与travler连接的socket，都将分装成 Snode (socket node)，
 * 包括连接traveller的client，或traveller主动连接的g_server
 */
#define SNODE_MAX_QUERYBUF_LEN (1024*1024*1024) /* 1GB max query buffer. */
#define SNODE_CLOSE_AFTER_REPLY (1<<0)  /* 发送完信息后，断开连接 */

typedef struct Snode_s {
    int flags;              /* SNODE_CLOSE_AFTER_REPLY | ... */
    int fd;
    char fds[16];           /* 字符串类型的fd */
    int recv_stat;          /* SNODE_RECV_STAT_ACCEPT ... */
    int recv_type;          /* SNODE_RECV_TYPE_ERR ... */
    sds tmp_querybuf;       /* 临时存放未解析完的argc 或 参数长度 */
    sds querybuf;           /* 读取到的数据 */
    sds writebuf;           /* 等待发送的数据 */
    int recv_parsing_stat;
    int argc;

    sds *argv;
    int argv_size;

    int argc_remaining;     /* 还剩多少个argc没有解析完成 */
    int argv_remaining;     /* 正在解析中的参数还有多少字符未获取 -1 为还没开始解析*/
    time_t lastinteraction; /* time of the last interaction, used for timeout */

    void (*proc)(struct Snode_s *sn);
    int is_write_mod;
} Snode;

#define SNODE_RECV_TYPE_ERR    -1 /* -:ERR */
#define SNODE_RECV_TYPE_OK     1  /* +:OK */
#define SNODE_RECV_TYPE_STRING 2
#define SNODE_RECV_TYPE_ARRAY  3  /* 数组 且 命令 */

struct trvCommand {
    char *key;
    void (*proc)(struct Snode_s *sn);
    int argc;
};

struct Server {
    time_t unixtime;        /* Unix time sampled every cron cycle. */

    int max_snodes;
    aeEventLoop *el;
    int port;
    int tcp_backlog;
    char neterr[ANET_ERR_LEN];   /* Error buffer for anet.c */
    int ipfd[2];                 /* 分别对应 ipv4、ipv6 */
    int ipfd_count;              /* 已绑定总量 */

    int stat_rejected_conn;
    int stat_numconnections;

    dict *snodes;                   /* key 是 fds */
    size_t snode_max_querybuf_len;  /* Limit for client query buffer length */

    int tcpkeepalive;

    Snode* current_snode;

    dict* commands;
};

int initServer(int port);
sds catSnodeInfoString(sds s, Snode *sn);
Snode* connectSnode(char *addr, int port);
void addReplyError(Snode *sn, char *err);
void addReplyStringArgv(Snode *sn, int argc, char **argv);
void addReplyMultiString(Snode *sn, int count, ...);
void addReplyMultiSds(Snode *sn, int count, ...);
void addReplySds(Snode *sn, sds data);
void addReplyRawSds(Snode *sn, sds data);
void addReplyString(Snode *sn, char *data);
void addReplyRawString(Snode *sn, char *data);
void freeSnode(Snode *sn);  /* dangerous */
Snode* getSnodeByFDS(const char *fds);

#endif
