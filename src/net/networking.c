#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "core/adlist.h"
#include "core/dict.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "net/networking.h"
#include "net/ae.h"
#include "net/anet.h"
#include "netcmd/netcmd.h"

extern struct NTServer g_server;
extern dictType stackStringTableDictType;

extern pthread_mutex_t g_blockNetWMtx;

static void setProtocolError(NTSnode *sn, int pos);
static void resetNTSnodeArgs(NTSnode *sn);
static int processInputBufferGetSegment(NTSnode *sn, sds *target_addr);
static int processInputBufferGetNum(NTSnode *sn, sds *target_addr, int *target_num);
static void processInputBufferStatus(NTSnode *sn);
static void processInputBufferString(NTSnode *sn);
static void processInputBufferArray(NTSnode *sn);
static void processInputBuffer(NTSnode *sn);
static void readQueryFromNTSnode(aeEventLoop *el, int fd, void *privdata, int mask);
static NTSnode* createNTSnode(int fd);
static void acceptCommonHandler(int fd, int flags);
static void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask);
static int listenToPort(int port, int *fds, int *count);
static void prepareNTSnodeToWrite(NTSnode *sn);
static void sendReplyToNTSnode(aeEventLoop *el, int fd, void *privdata, int mask);
static NTSnode* snodeArgvMakeRoomFor(NTSnode *sn, int count);
static NTSnode* snodeArgvClear(NTSnode *sn);
static NTSnode* snodeArgvEmpty(NTSnode *sn);
static NTSnode* snodeArgvFree(NTSnode *sn);

static NTSnode* snodeArgvFree(NTSnode *sn) {
    if (NULL == sn->argv) return sn;

    int loopJ;
    for (loopJ = 0; loopJ < sn->argv_size; loopJ++) {
        sdsfree(sn->argv[loopJ]);
    }
    zfree(sn->argv);
    sn->argv = NULL;
    return sn;
}

static NTSnode* snodeArgvEmpty(NTSnode *sn) {
    sn->argv = NULL;
    sn->argv_size = 0;
    return sn;
}

static NTSnode* snodeArgvMakeRoomFor(NTSnode *sn, int count) {
    if (sn->argv_size >= count) return sn;

    int loopJ;

    if (NULL == sn->argv) sn->argv = zmalloc(sizeof(sds*) * count);
    else sn->argv = zrealloc(sn->argv, sizeof(sds*) * count);

    for (loopJ = sn->argv_size; loopJ < count; loopJ++) {
        sn->argv[loopJ] = sdsempty();
    }
    sn->argv_size = count;

    return sn;
}

static NTSnode* snodeArgvClear(NTSnode *sn) {
    if (0 == sn->argv_size) return sn;

    int loopJ;
    for (loopJ = 0; loopJ < sn->argv_size; loopJ++) {
        sdsclear(sn->argv[loopJ]);
    }

    return sn;
}

static void prepareNTSnodeToWrite(NTSnode *sn) {
    if (sn->is_write_mod) return;

    aeCreateFileEvent(g_server.el, sn->fd, AE_WRITABLE, sendReplyToNTSnode, sn);
    sn->is_write_mod = 1;
}

static void sendReplyToNTSnode(aeEventLoop *el, int fd, void *privdata, int mask) {
    int nwritten = 0;
    int bufpos = 0;
    NTSnode *sn = privdata;
    int tmpsize = sdslen(sn->writebuf);

    while(bufpos < tmpsize) {
        nwritten = write(fd, sn->writebuf+bufpos, tmpsize-nwritten);
        if (nwritten <= 0) break;
        bufpos += nwritten;
    }

    if (bufpos > 0) sdsrange(sn->writebuf, bufpos, -1);

    if (0 == sdslen(sn->writebuf)) {
        aeDeleteFileEvent(g_server.el, sn->fd, AE_WRITABLE);
        sn->is_write_mod = 0;

        if (sn->flags & SNODE_CLOSE_AFTER_REPLY) NTFreeNTSnode(sn);
    }
}

void NTAddReplySds(NTSnode *sn, sds data) {
    pthread_mutex_lock(&g_blockNetWMtx);
    prepareNTSnodeToWrite(sn);
    sn->writebuf = sdscatfmt(sn->writebuf, "$%u\r\n%s\r\n", (unsigned int)sdslen(data), data);
    pthread_mutex_unlock(&g_blockNetWMtx);
}

void NTAddReplyRawSds(NTSnode *sn, sds data) {
    pthread_mutex_lock(&g_blockNetWMtx);
    prepareNTSnodeToWrite(sn);
    sn->writebuf = sdscatsds(sn->writebuf, data);
    pthread_mutex_unlock(&g_blockNetWMtx);
}

void NTAddReplyString(NTSnode *sn, char *data) {
    pthread_mutex_lock(&g_blockNetWMtx);
    prepareNTSnodeToWrite(sn);
    sn->writebuf = sdscatfmt(sn->writebuf, "$%u\r\n%s\r\n", (unsigned int)strlen(data), data);
    pthread_mutex_unlock(&g_blockNetWMtx);
}

void NTAddReplyRawString(NTSnode *sn, char *data) {
    pthread_mutex_lock(&g_blockNetWMtx);
    prepareNTSnodeToWrite(sn);
    sn->writebuf = sdscat(sn->writebuf, data);
    pthread_mutex_unlock(&g_blockNetWMtx);
}

void NTAddReplyMultiSds(NTSnode *sn, int count, ...) {
    pthread_mutex_lock(&g_blockNetWMtx);
    prepareNTSnodeToWrite(sn);
    va_list ap;
    sds tmpptr;
    int loopJ;

    va_start(ap, count);
    sn->writebuf = sdscatfmt(sn->writebuf, "*%i\r\n", count);
    for (loopJ = 0; loopJ < count; loopJ++) {
        tmpptr = va_arg(ap, sds);
        sn->writebuf = sdscatfmt(sn->writebuf, "$%d\r\n%S\r\n", sdslen(tmpptr), tmpptr);
    }
    va_end(ap); 
    pthread_mutex_unlock(&g_blockNetWMtx);
}


void NTAddReplyStringArgv(NTSnode *sn, int argc, char **argv) {
    pthread_mutex_lock(&g_blockNetWMtx);
    prepareNTSnodeToWrite(sn);
    int loopJ;
    sn->writebuf = sdscatfmt(sn->writebuf, "*%i\r\n", argc);
    for (loopJ = 0; loopJ < argc; loopJ++) {
        sn->writebuf = sdscatfmt(sn->writebuf, "$%i\r\n%s\r\n", strlen(argv[loopJ]), argv[loopJ]);
    }
    pthread_mutex_unlock(&g_blockNetWMtx);
}


void NTAddReplyMultiString(NTSnode *sn, int count, ...) {
    pthread_mutex_lock(&g_blockNetWMtx);
    prepareNTSnodeToWrite(sn);
    va_list ap;
    sds tmpptr;
    int loopJ;

    va_start(ap, count);
    sn->writebuf = sdscatfmt(sn->writebuf, "*%i\r\n", count);
    for (loopJ = 0; loopJ < count; loopJ++) {
        tmpptr = va_arg(ap, char*);
        sn->writebuf = sdscatfmt(sn->writebuf, "$%i\r\n%s\r\n", strlen(tmpptr), tmpptr);
    }
    va_end(ap); 
    pthread_mutex_unlock(&g_blockNetWMtx);
}

void NTAddReplyError(NTSnode *sn, char *err) {
    pthread_mutex_lock(&g_blockNetWMtx);
    prepareNTSnodeToWrite(sn);
    sn->writebuf = sdscatlen(sn->writebuf, "-", 1);
    sn->writebuf = sdscat(sn->writebuf, err);
    sn->writebuf = sdscatlen(sn->writebuf, "\r\n", 2);
    pthread_mutex_unlock(&g_blockNetWMtx);
}


sds NTCatNTSnodeInfoString(sds s, NTSnode *sn) {
    return sdscatfmt(s, "NTSnode");
}


static void setProtocolError(NTSnode *sn, int pos) {
    sn->flags |= SNODE_CLOSE_AFTER_REPLY;
    sdsrange(sn->querybuf,pos,-1);
}


/* 重新使 NTSnode 准备就绪 可以读取新的指令
 * 该函数在 processInputBuffer 完成时调用
 */
static void rePrepareNTSnodeToReadQuery(NTSnode *sn) {
    sn->recv_stat = SNODE_RECV_STAT_PREPARE;
    sn->recv_type = ERRNO_NULL;
    sn->recv_parsing_stat = ERRNO_NULL;

    if (NULL == sn->tmp_querybuf) sn->tmp_querybuf = sdsempty();
    else sdsclear(sn->tmp_querybuf);

    sn->argc = 0;
    sn = snodeArgvClear(sn);

    sn->argc_remaining = 0;
    sn->argv_remaining = 0;
    sn->proc = NULL;
}

static void resetNTSnodeArgs(NTSnode *sn) {
    sn->flags = ERRNO_NULL;
    sn->recv_stat = ERRNO_NULL;
    sn->recv_type = ERRNO_NULL;
    sn->recv_parsing_stat = ERRNO_NULL;

    sdsclear(sn->tmp_querybuf);
    sdsclear(sn->querybuf);
    sdsclear(sn->writebuf);

    sn = snodeArgvClear(sn);
    sn->argc = 0;

    sn->argc_remaining = 0;
    sn->argv_remaining = 0;
    sn->proc = NULL;
}

/* 移动 querybuf 到 target_addr 一直到遇到 \r\n，或者最后一个\n，
 * 如果一直没遇到，则直接读取所有
 */
static int processInputBufferGetSegment(NTSnode *sn, sds *target_addr) {
    char *newline;
    int offset;
    int readlen = 0;

    while (1) {
        if (sdslen(sn->querybuf) <= 0) break;

        newline = strchr(sn->querybuf, '\n');
        
        if (NULL == newline) {
            *target_addr = sdscatsds(*target_addr, sn->querybuf);
            sdsclear(sn->querybuf);
            return readlen;
        }

        offset = newline - sn->querybuf + 1;
        readlen += offset;
        *target_addr = sdscatlen(*target_addr, sn->querybuf, offset);
        sdsrange(sn->querybuf, offset, -1);

        /* target_addr 尾部必然是 \n ，如果倒数第二个是\r
         * 则已读取到 \r\n ，则读取完成
         */
        if ('\r' == (*target_addr)[sdslen(*target_addr)-2]) return EOF;
    }

    return readlen;
}


/* 从querybuf中读取数字，并使用tmp_querybuf
 */
static int processInputBufferGetNum(NTSnode *sn, sds *target_addr, int *target_num) {
    int readlen = 1;
    readlen = processInputBufferGetSegment(sn, target_addr);

    if (sdslen(*target_addr) > 10) { 
        *target_num = -1;
        return EOF; 
    }

    if (EOF == readlen) {
        sdsrange(*target_addr, 0, -3);
        char tmpstr[12];
        memcpy(tmpstr, *target_addr, sdslen(*target_addr));
        tmpstr[sdslen(*target_addr)] = '\0';
        *target_num = atoi(tmpstr);
    }

    return readlen;
}


static void processInputBufferStatus(NTSnode *sn) {
    int readlen = 0;

    trvAssert(SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat);

    if (SNODE_RECV_STAT_PARSING_START == sn->recv_parsing_stat) {
        sn->recv_parsing_stat = SNODE_RECV_STAT_PARSING_ARGV_VALUE;
        sn->argc = 1;
        sn = snodeArgvMakeRoomFor(sn, 1);
        sn = snodeArgvClear(sn);
        readlen = processInputBufferGetSegment(sn, &(sn->argv[0]));
    }

    else if (SNODE_RECV_STAT_PARSING_ARGV_VALUE == sn->recv_parsing_stat) {
        readlen = processInputBufferGetSegment(sn, &(sn->argv[0]));
    }

    if (EOF == readlen) {
        sdsrange(sn->argv[0], 0, -3);
        sn->recv_parsing_stat = SNODE_RECV_STAT_PARSING_FINISHED;
        sn->recv_stat = SNODE_RECV_STAT_PARSED;
    }

}


static void processInputBufferString(NTSnode *sn) {
    int readlen = 0, len;

    trvAssert(SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat);

    if (SNODE_RECV_STAT_PARSING_START == sn->recv_parsing_stat) {
        sdsclear(sn->tmp_querybuf);

        sn->recv_parsing_stat = SNODE_RECV_STAT_PARSING_ARGV_NUM;
        sn->argc = 1;
        sn = snodeArgvMakeRoomFor(sn, 1);
        sn = snodeArgvClear(sn);

    }

    if (SNODE_RECV_STAT_PARSING_ARGV_NUM == sn->recv_parsing_stat) {
        sdsrange(sn->querybuf, 1, -1);//skip $
        readlen = processInputBufferGetNum(sn, &(sn->tmp_querybuf), &(sn->argv_remaining));

        if (EOF != readlen) return;

        if (sn->argv_remaining < 0) {
            setProtocolError(sn, 0);
            return;
        }

        sn->recv_parsing_stat = SNODE_RECV_STAT_PARSING_ARGV_VALUE;

        sdsclear(sn->tmp_querybuf);
    }

    if (SNODE_RECV_STAT_PARSING_ARGV_VALUE == sn->recv_parsing_stat) {
        while(1) {

            len = sdslen(sn->querybuf);

            /* querybuf少于需要读取的，读取所有querybuf，并返回 */
            if (sn->argv_remaining > len) {
                sn->argv[0] = sdscatlen(sn->argv[0], sn->querybuf, len);
                sdsclear(sn->querybuf);
                sn->argv_remaining -= len;
                return;
            }

            /* querybuf中有足够数据，则读取，并设置为读取完成 */
            sn->argv[0] = sdscatlen(sn->argv[0], sn->querybuf, sn->argv_remaining);

            if (len == sn->argv_remaining) sdsclear(sn->querybuf);
            else sdsrange(sn->querybuf, sn->argv_remaining, -1);

            sn->argv_remaining = 0;
            sn->recv_parsing_stat = SNODE_RECV_STAT_PARSING_FINISHED;
            sn->recv_stat = SNODE_RECV_STAT_PARSED;
        }

    }
}


static void processInputBufferArray(NTSnode *sn) {
    int readlen = 0, len, argJ;

    trvAssert(SNODE_RECV_STAT_PARSING_FINISHED != sn->recv_parsing_stat);

    if (SNODE_RECV_STAT_PARSING_START == sn->recv_parsing_stat) {
        sdsclear(sn->tmp_querybuf);

        sn->recv_parsing_stat = SNODE_RECV_STAT_PARSING_ARGC;
        sn->argc = 0;
        sn = snodeArgvClear(sn);

    }

    if (SNODE_RECV_STAT_PARSING_ARGC == sn->recv_parsing_stat) {
        readlen = processInputBufferGetNum(sn, &(sn->tmp_querybuf), &(sn->argc));
        
        if (EOF != readlen) return;

        if (sn->argc <= 0) {
            setProtocolError(sn, 0);
            return;
        }

        sn = snodeArgvMakeRoomFor(sn, sn->argc);
        sn = snodeArgvClear(sn);
        sn->argc_remaining = sn->argc;

        sdsclear(sn->tmp_querybuf);
        sn->recv_parsing_stat = SNODE_RECV_STAT_PARSING_ARGV_NUM;
    }


PARSING_ARGV_START:

    if (0 == sn->argc_remaining) {
        sn->recv_parsing_stat = SNODE_RECV_STAT_PARSING_FINISHED;
        sn->recv_stat = SNODE_RECV_STAT_PARSED;
        return;
    }

    if (0 == sdslen(sn->querybuf)) return;

//PARSING_ARGV_NUM:
    if (SNODE_RECV_STAT_PARSING_ARGV_NUM == sn->recv_parsing_stat) {
        sdsrange(sn->querybuf, 1, -1);//skip $
        readlen = processInputBufferGetNum(sn, &(sn->tmp_querybuf), &(sn->argv_remaining));

        if (EOF != readlen) return;

        if (sn->argv_remaining < 0) {
            setProtocolError(sn, 0);
            return;
        }

        sn->argv_remaining += 2; //增加读取 \r\n

        sdsclear(sn->tmp_querybuf);
        sn->recv_parsing_stat = SNODE_RECV_STAT_PARSING_ARGV_VALUE;

    }

//PARSING_ARGV_VALUE:
    if (SNODE_RECV_STAT_PARSING_ARGV_VALUE == sn->recv_parsing_stat) {
        argJ = sn->argc - sn->argc_remaining;

        len = sdslen(sn->querybuf);

        /* querybuf少于需要读取的，读取所有querybuf，并返回 */
        if (sn->argv_remaining > len) {
            sn->argv[argJ] = sdscatlen(sn->argv[argJ], sn->querybuf, len);
            sdsclear(sn->querybuf);
            sn->argv_remaining -= len;
            return;
        }

        /* querybuf中有足够数据，则读取，并设置为读取完成 */
        sn->argv[argJ] = sdscatlen(sn->argv[argJ], sn->querybuf, sn->argv_remaining);

        if (len == sn->argv_remaining) sdsclear(sn->querybuf);
        else sdsrange(sn->querybuf, sn->argv_remaining, -1);


        sn->argv_remaining = 0;
        sn->recv_parsing_stat = SNODE_RECV_STAT_PARSING_ARGV_NUM;

        sn->argc_remaining--;

        sdsrange(sn->argv[argJ], 0, -3);//去掉 \r\n

        goto PARSING_ARGV_START;
    }

}


/* 解析读取数据
 */
static void processInputBuffer(NTSnode *sn) {
    if (SNODE_RECV_STAT_ACCEPT == sn->recv_stat || SNODE_RECV_STAT_PREPARE == sn->recv_stat) {

        switch(sn->querybuf[0]) {
            case '+' :
                sn->recv_type = SNODE_RECV_TYPE_OK;
                break;

            case '-' :
                sn->recv_type = SNODE_RECV_TYPE_ERR;
                break;

            case '$' :
                sn->recv_type = SNODE_RECV_TYPE_STRING;
                break;

            case '*' :
                sn->recv_type = SNODE_RECV_TYPE_ARRAY;
                break;

            default :
                NTAddReplyError(sn, "Protocol error: Unrecognized type");
                setProtocolError(sn, 0);
                return;
        }

        sdsrange(sn->querybuf, 1, -1);
        sn->recv_stat = SNODE_RECV_STAT_PARSING;
        sn->recv_parsing_stat = SNODE_RECV_STAT_PARSING_START;
    }

    if (SNODE_RECV_TYPE_OK == sn->recv_type || SNODE_RECV_TYPE_ERR == sn->recv_type) {
        processInputBufferStatus(sn);
    }
    else if (SNODE_RECV_TYPE_STRING == sn->recv_type) {
        processInputBufferString(sn);
    }
    else if (SNODE_RECV_TYPE_ARRAY == sn->recv_type) {
        processInputBufferArray(sn);

        if (NULL == sn->proc && sn->argc - sn->argc_remaining > 0) {
            dictEntry *de;

            de = dictFind(g_server.commands, sn->argv[0]);
            if (NULL == de) {
                NTAddReplyError(sn, "command unrecognized");
                setProtocolError(sn, 0);
                return;
            }
            sn->proc = dictGetVal(de);
            sn->recv_stat = SNODE_RECV_STAT_EXCUTING;
        }
    }
    else {
        NTAddReplyError(sn, "Protocol error: Unrecognized");
        setProtocolError(sn, 0);
        return;
    }

    if (NULL == sn->proc) {
        if (SNODE_RECV_STAT_PARSED == sn->recv_stat) {
            rePrepareNTSnodeToReadQuery(sn);
        }
    }
    else {
        sn->proc(sn);

        if (SNODE_RECV_STAT_EXCUTED == sn->recv_stat) {
            rePrepareNTSnodeToReadQuery(sn);
        }
    }
}


static void readQueryFromNTSnode(aeEventLoop *el, int fd, void *privdata, int mask) {
    NTSnode *sn = (NTSnode*) privdata;
    int nread;
    NOTUSED(el);
    NOTUSED(mask);

    if (SNODE_CLOSE_AFTER_REPLY == sn->flags) {
        return;
    }

    if (sdsavail(sn->querybuf) < TRV_NET_IOBUF_LEN+2) {
        sn->querybuf = sdsMakeRoomFor(sn->querybuf, sdslen(sn->querybuf) + TRV_NET_IOBUF_LEN +2);
    }

    g_server.current_snode = sn;

    nread = read(fd, sn->querybuf+sdslen(sn->querybuf), TRV_NET_IOBUF_LEN);
    if (nread >= 0) {
        sn->querybuf[sdslen(sn->querybuf)+nread] = 0;
        sdsupdatelen(sn->querybuf);
    }

    if (-1 == nread) {
        if (EAGAIN == errno) {
            nread = 0;
        } 
        else {
            trvLogD("Reading from client: %s",strerror(errno));
            NTFreeNTSnode(sn);
        }
        return;
    } 
    else if (0 == nread) {
        NTFreeNTSnode(sn);
        return;
    }
    processInputBuffer(sn);
}


static NTSnode* createNTSnode(int fd) {
    trvLogD("Create snode %d", fd);

    NTSnode *sn = zmalloc(sizeof(NTSnode));

    if (-1 == fd) {
        return NULL;
    }

    anetNonBlock(NULL,fd);
    anetEnableTcpNoDelay(NULL,fd);
    if (g_server.tcpkeepalive)
        anetKeepAlive(NULL,fd,g_server.tcpkeepalive);
    if (aeCreateFileEvent(g_server.el,fd,AE_READABLE,
                readQueryFromNTSnode, sn) == AE_ERR)
    {
        close(fd);
        zfree(sn);
        return NULL;
    }

    sn->fd = fd;
    sprintf(sn->fds, "%d", sn->fd);
    sn->tmp_querybuf = sdsempty();
    sn->querybuf = sdsempty();
    sn->writebuf = sdsempty();
    sn = snodeArgvEmpty(sn);
    sn->is_write_mod = 0;
    resetNTSnodeArgs(sn);

    sn->recv_stat = SNODE_RECV_STAT_ACCEPT;

    g_server.stat_numconnections++;
    dictAdd(g_server.snodes, sn->fds, sn);

    return sn;
}



void NTFreeNTSnode(NTSnode *sn) {
    trvLogD("Free NTSnode %d", sn->fd);

    if (-1 != sn->fd) {
        aeDeleteFileEvent(g_server.el, sn->fd, AE_READABLE);
        aeDeleteFileEvent(g_server.el, sn->fd, AE_WRITABLE);
        close(sn->fd);
    }

    sn = snodeArgvFree(sn);
    sdsfree(sn->tmp_querybuf);
    sdsfree(sn->querybuf);
    sdsfree(sn->writebuf);

    dictDelete(g_server.snodes, sn->fds);

    zfree(sn);

    g_server.stat_numconnections--;
}

NTSnode *NTConnectNTSnode(char *addr, int port) {
    int fd;
    NTSnode *sn;

    memset(g_server.neterr, 0x00, ANET_ERR_LEN);
    fd = anetPeerSocket(g_server.neterr, 0, "0.0.0.0", AF_INET);
    fd = anetPeerConnect(fd, g_server.neterr, addr, port);
    //fd = anetPeerConnect(fd, g_server.neterr, addr, port);
    if (ANET_ERR == fd) {
        trvLogW("Unable to connect to %s", addr);
        return NULL;
    }
    
    sn = createNTSnode(fd);
    return sn;
}

static void acceptCommonHandler(int fd, int flags) {
    NTSnode *sn;
    if (NULL == (sn = createNTSnode(fd))) {
        trvLogW("Error registering fd event for the new snode: %s (fd=%d)",
                strerror(errno),fd);
        close(fd); /* May be already closed, just ignore errors */
        return;
    }

    if (dictSize(g_server.snodes) > g_server.max_snodes) {
        char *err = "-ERR max number of snodes reached\r\n";

        /* That's a best effort error message, don't check write errors */
        if (write(sn->fd,err,strlen(err)) == -1) {
            /* Nothing to do, Just to avoid the warning... */
        }
        g_server.stat_rejected_conn++;
        NTFreeNTSnode(sn);
        return;
    }
    sn->flags |= flags;
}



static void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    int cport, cfd, max = TRV_NET_MAX_ACCEPTS_PER_CALL;
    char cip[INET6_ADDRSTRLEN];
    NOTUSED(el);
    NOTUSED(mask);
    NOTUSED(privdata);

    while(max--) {
        cfd = anetTcpAccept(g_server.neterr, fd, cip, sizeof(cip), &cport);
        if (cfd == ANET_ERR) {
            if (errno != EWOULDBLOCK)
                trvLogW("Accepting snode connection: %s", g_server.neterr);
            return;
        }
        trvLogD("Accepted %s:%d", cip, cport);
        acceptCommonHandler(cfd,0);
    }
}


static int listenToPort() {
    int loopJ; 

    g_server.ipfd_count = 0;

    for (loopJ = 0; loopJ < g_server.ipfd_count; loopJ++) {
        g_server.ipfd[loopJ] = ANET_ERR;
    }

    if (g_server.ipfd[0] != ANET_ERR) {
        anetPeerListen(g_server.ipfd[0], g_server.neterr, g_server.tcp_backlog);
        g_server.ipfd_count++;
    }

    if (g_server.ipfd[1] != ANET_ERR) {
        anetPeerListen(g_server.ipfd[1], g_server.neterr, g_server.tcp_backlog);
        g_server.ipfd_count++;
    }

    if (0 == g_server.ipfd_count) return ERRNO_ERR;

    for (loopJ = 0; loopJ < g_server.ipfd_count; loopJ++) {
        if (aeCreateFileEvent(g_server.el, g_server.ipfd[loopJ], AE_READABLE,
                    acceptTcpHandler,NULL) == AE_ERR) {
            trvLogE("Unrecoverable error creating g_server.ipfd file event.");
        }
    }

    trvLogI("监听端口: %d", g_server.port);

    return ERRNO_OK;
}

NTSnode* NTGetNTSnodeByFDS(const char *fds) {
    dictEntry *de;

    de = dictFind(g_server.snodes, fds);
    if (NULL == de) {
        return NULL;
    }

    return dictGetVal(de);
}

int NTInit(int listenPort) {
    g_server.unixtime = -1;
    g_server.max_snodes = TRV_NET_MAX_SNODE;
    g_server.el = aeCreateEventLoop(g_server.max_snodes);

    g_server.bindaddr = "0.0.0.0";
    g_server.port = listenPort;
    g_server.tcp_backlog = TRV_NET_TCP_BACKLOG;

    g_server.stat_numconnections = 0;
    g_server.stat_rejected_conn = 0;

    g_server.snodes = dictCreate(&stackStringTableDictType, NULL);
    g_server.snode_max_querybuf_len = SNODE_MAX_QUERYBUF_LEN;

    g_server.tcpkeepalive = TRV_NET_TCPKEEPALIVE;

    g_server.ipfd[0] = anetPeerSocket(g_server.neterr, g_server.port, g_server.bindaddr, AF_INET);
    g_server.ipfd[1] = anetPeerSocket(g_server.neterr, g_server.port, g_server.bindaddr, AF_INET6);

    if (listenPort > 0) {
        if (ERRNO_ERR == listenToPort(g_server.port, g_server.ipfd, &g_server.ipfd_count)) {
            trvLogE("Listen to port err");
            return ERRNO_ERR;
        }
    }

    initNetCmd();

    return ERRNO_OK;
}
