#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>

#include "core/adlist.h"
#include "core/dict.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "net/networking.h"
#include "net/anet.h"
#include "event/event.h"
#include "service/service.h"
#include "script/script.h"
#include "core/extern.h"

aeLooper *nt_el;
NTServer nt_server;

static void setProtocolError(NTSnode *sn, int pos);
static void resetNTSnodeArgs(NTSnode *sn);
static int parseInputBufferGetSegment(NTSnode *sn, sds *target_addr);
static int parseInputBufferGetNum(NTSnode *sn, sds *target_addr, int *target_num);
static void parseInputBufferStatus(NTSnode *sn);
static void parseInputBufferString(NTSnode *sn);
static void parseInputBufferArray(NTSnode *sn);
static void parseInputBuffer(NTSnode *sn);
static void readQueryFromNTSnode(aeLooper *el, int fd, void *privdata, int mask);
static NTSnode* createNTSnode(int fd);
static void acceptCommonHandler(int fd, int flags);
static void acceptTcpHandler(aeLooper *el, int fd, void *privdata, int mask);
static int listenToPort(int port, int *fds, int *count);
static void prepareNTSnodeToWrite(NTSnode *sn);
static void sendReplyToNTSnode(aeLooper *el, int fd, void *privdata, int mask);
static NTSnode* snodeArgvMakeRoomFor(NTSnode *sn, int count);
static NTSnode* snodeArgvClear(NTSnode *sn);
static NTSnode* snodeArgvEmpty(NTSnode *sn);
static NTSnode* snodeArgvFree(NTSnode *sn);

static NTSnode* snodeArgvFree(NTSnode *sn) {
    if (0 == sn->argv) return sn;

    int loopJ;
    for (loopJ = 0; loopJ < sn->argvSize; loopJ++) {
        sdsfree(sn->argv[loopJ]);
    }
    zfree(sn->argv);
    sn->argv = 0;
    return sn;
}

static NTSnode* snodeArgvEmpty(NTSnode *sn) {
    sn->argv = 0;
    sn->argvSize = 0;
    return sn;
}

static NTSnode* snodeArgvMakeRoomFor(NTSnode *sn, int count) {
    if (sn->argvSize >= count) return sn;

    int loopJ;

    if (0 == sn->argv) {
        sn->argv = zmalloc(sizeof(sds*) * count);
    } else {
        sn->argv = zrealloc(sn->argv, sizeof(sds*) * count);
    }

    for (loopJ = sn->argvSize; loopJ < count; loopJ++) {
        sn->argv[loopJ] = sdsempty();
    }
    sn->argvSize = count;

    return sn;
}

static NTSnode* snodeArgvClear(NTSnode *sn) {
    if (0 == sn->argvSize) return sn;

    int loopJ;
    for (loopJ = 0; loopJ < sn->argvSize; loopJ++) {
        sdsclear(sn->argv[loopJ]);
    }

    return sn;
}

static void prepareNTSnodeToWrite(NTSnode *sn) {
    if (sn->isWriteMod) return;

    aeCreateFileEvent(nt_el, sn->fd, AE_WRITABLE, sendReplyToNTSnode, sn);
    sn->isWriteMod = 1;
}

static void sendReplyToNTSnode(aeLooper *el, int fd, void *privdata, int mask) {
    int nwritten = 0;
    int bufpos = 0;
    NTSnode *sn = privdata;
    int tmpsize = sdslen(sn->writebuf);

    while(bufpos < tmpsize) {
        nwritten = write(fd, sn->writebuf+bufpos, tmpsize-nwritten);
        if (nwritten <= 0) {
            if (EAGAIN == errno) {
                nwritten = 0;
            } else {
                sn->recvType = SNODE_RECV_TYPE_HUP;
                if (0 != sn->hupProc) {
                    sn->hupProc(sn);
                }
                if (0 != sn->proc) {
                    sn->proc(sn);
                }
                NTFreeNTSnode(sn);
                return;
            }
        }
        bufpos += nwritten;
    }

    if (bufpos > 0) sdsrange(sn->writebuf, bufpos, -1);

    if (0 == sdslen(sn->writebuf)) {
        aeDeleteFileEvent(nt_el, sn->fd, AE_WRITABLE);
        sn->isWriteMod = 0;

        if (sn->flags & SNODE_CLOSE_AFTER_REPLY) {
            NTFreeNTSnode(sn);
        }
    }
}

void NTAddReplySds(NTSnode *sn, sds data) {
    prepareNTSnodeToWrite(sn);
    sn->writebuf = sdscatfmt(sn->writebuf, "$%u\r\n%s\r\n", (unsigned int)sdslen(data), data);
}

void NTAddReplyRawSds(NTSnode *sn, sds data) {
    prepareNTSnodeToWrite(sn);
    sn->writebuf = sdscatsds(sn->writebuf, data);
}

void NTAddReplyString(NTSnode *sn, char *data) {
    prepareNTSnodeToWrite(sn);
    sn->writebuf = sdscatfmt(sn->writebuf, "$%u\r\n%s\r\n", (unsigned int)strlen(data), data);
}

void NTAddReplyRawString(NTSnode *sn, char *data) {
    prepareNTSnodeToWrite(sn);
    sn->writebuf = sdscat(sn->writebuf, data);
}

void NTAddReplyMultiSds(NTSnode *sn, int count, ...) {
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
}


void NTAddReplyStringArgv(NTSnode *sn, int argc, char **argv) {
    prepareNTSnodeToWrite(sn);
    int loopJ;
    sn->writebuf = sdscatfmt(sn->writebuf, "*%i\r\n", argc);
    for (loopJ = 0; loopJ < argc; loopJ++) {
        sn->writebuf = sdscatfmt(sn->writebuf, "$%i\r\n%s\r\n", strlen(argv[loopJ]), argv[loopJ]);
    }
}


void NTAddReplyMultiString(NTSnode *sn, int count, ...) {
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
}

void NTAddReplyError(NTSnode *sn, char *err) {
    prepareNTSnodeToWrite(sn);
    sn->writebuf = sdscatlen(sn->writebuf, "-", 1);
    sn->writebuf = sdscat(sn->writebuf, err);
    sn->writebuf = sdscatlen(sn->writebuf, "\r\n", 2);
}


sds NTCatNTSnodeInfoString(sds s, NTSnode *sn) {
    return sdscatfmt(s, "NTSnode");
}


static void setProtocolError(NTSnode *sn, int pos) {
    sn->flags |= SNODE_CLOSE_AFTER_REPLY;
    sdsrange(sn->querybuf,pos,-1);
}


/* 重新使 NTSnode 准备就绪 可以读取新的指令
 * 该函数在 parseInputBuffer 完成时调用
 */
static void rePrepareNTSnodeToReadQuery(NTSnode *sn) {
    sn->recvStat = SNODE_RECV_STAT_PREPARE;
    sn->recvType = ERRNO_NULL;
    sn->recvParsingStat = ERRNO_NULL;

    if (0 == sn->tmpQuerybuf) {
        sn->tmpQuerybuf = sdsempty();
    } else {
        sdsclear(sn->tmpQuerybuf);
    }

    sn->argc = 0;
    sn = snodeArgvClear(sn);

    sn->argcRemaining = 0;
    sn->argvRemaining = 0;
    sn->proc = 0;
}

static void resetNTSnodeArgs(NTSnode *sn) {
    sn->flags = ERRNO_NULL;
    sn->recvStat = ERRNO_NULL;
    sn->recvType = ERRNO_NULL;
    sn->recvParsingStat = ERRNO_NULL;

    sdsclear(sn->tmpQuerybuf);
    sdsclear(sn->querybuf);
    sdsclear(sn->writebuf);

    sn = snodeArgvClear(sn);
    sn->argc = 0;

    sn->argcRemaining = 0;
    sn->argvRemaining = 0;
    sn->proc = 0;
}

/* 移动 querybuf 到 target_addr 一直到遇到 \r\n，或者最后一个\n，
 * 如果一直没遇到，则直接读取所有
 */
static int parseInputBufferGetSegment(NTSnode *sn, sds *target_addr) {
    char *newline;
    int offset;
    int readlen = 0;

    while (1) {
        if (sdslen(sn->querybuf) <= 0) break;

        newline = strchr(sn->querybuf, '\n');
        
        if (0 == newline) {
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

/* 从querybuf中读取数字，并使用tmpQuerybuf
 */
static int parseInputBufferGetNum(NTSnode *sn, sds *target_addr, int *target_num) {
    int readlen = 1;
    readlen = parseInputBufferGetSegment(sn, target_addr);

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

static void parseInputBufferStatus(NTSnode *sn) {
    int readlen = 0;

    TrvAssert(SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat, "parseInputBufferStatus error");

    if (SNODE_RECV_STAT_PARSING_START == sn->recvParsingStat) {
        sn->recvParsingStat = SNODE_RECV_STAT_PARSING_ARGV_VALUE;
        sn->argc = 1;
        sn = snodeArgvMakeRoomFor(sn, 1);
        sn = snodeArgvClear(sn);
        readlen = parseInputBufferGetSegment(sn, &(sn->argv[0]));

    } else if (SNODE_RECV_STAT_PARSING_ARGV_VALUE == sn->recvParsingStat) {
        readlen = parseInputBufferGetSegment(sn, &(sn->argv[0]));
    }

    if (EOF == readlen) {
        sdsrange(sn->argv[0], 0, -3);
        sn->recvParsingStat = SNODE_RECV_STAT_PARSING_FINISHED;
        sn->recvStat = SNODE_RECV_STAT_PARSED;
    }

}

static void parseInputBufferString(NTSnode *sn) {
    int readlen = 0, len;

    TrvAssert(SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat, "parseInputBufferString error");

    if (SNODE_RECV_STAT_PARSING_START == sn->recvParsingStat) {
        sdsclear(sn->tmpQuerybuf);

        sn->recvParsingStat = SNODE_RECV_STAT_PARSING_ARGV_NUM;
        sn->argc = 1;
        sn = snodeArgvMakeRoomFor(sn, 1);
        sn = snodeArgvClear(sn);

    }

    if (SNODE_RECV_STAT_PARSING_ARGV_NUM == sn->recvParsingStat) {
        sdsrange(sn->querybuf, 1, -1);//skip $
        readlen = parseInputBufferGetNum(sn, &(sn->tmpQuerybuf), &(sn->argvRemaining));

        if (EOF != readlen) return;

        if (sn->argvRemaining < 0) {
            setProtocolError(sn, 0);
            return;
        }

        sn->recvParsingStat = SNODE_RECV_STAT_PARSING_ARGV_VALUE;

        sdsclear(sn->tmpQuerybuf);
    }

    if (SNODE_RECV_STAT_PARSING_ARGV_VALUE == sn->recvParsingStat) {
        while(1) {

            len = sdslen(sn->querybuf);

            /* querybuf少于需要读取的，读取所有querybuf，并返回 */
            if (sn->argvRemaining > len) {
                sn->argv[0] = sdscatlen(sn->argv[0], sn->querybuf, len);
                sdsclear(sn->querybuf);
                sn->argvRemaining -= len;
                return;
            }

            /* querybuf中有足够数据，则读取，并设置为读取完成 */
            sn->argv[0] = sdscatlen(sn->argv[0], sn->querybuf, sn->argvRemaining);

            if (len == sn->argvRemaining) {
                sdsclear(sn->querybuf);
            } else {
                sdsrange(sn->querybuf, sn->argvRemaining, -1);
            }

            sn->argvRemaining = 0;
            sn->recvParsingStat = SNODE_RECV_STAT_PARSING_FINISHED;
            sn->recvStat = SNODE_RECV_STAT_PARSED;
        }

    }
}

static void parseInputBufferArray(NTSnode *sn) {
    int readlen = 0, len, argJ;

    TrvAssert(SNODE_RECV_STAT_PARSING_FINISHED != sn->recvParsingStat, "parseInputBufferArray error");

    if (SNODE_RECV_STAT_PARSING_START == sn->recvParsingStat) {
        sdsclear(sn->tmpQuerybuf);

        sn->recvParsingStat = SNODE_RECV_STAT_PARSING_ARGC;
        sn->argc = 0;
        sn = snodeArgvClear(sn);

    }

    if (SNODE_RECV_STAT_PARSING_ARGC == sn->recvParsingStat) {
        readlen = parseInputBufferGetNum(sn, &(sn->tmpQuerybuf), &(sn->argc));
        
        if (EOF != readlen) return;

        if (sn->argc <= 0) {
            setProtocolError(sn, 0);
            return;
        }

        sn = snodeArgvMakeRoomFor(sn, sn->argc);
        sn = snodeArgvClear(sn);
        sn->argcRemaining = sn->argc;

        sdsclear(sn->tmpQuerybuf);
        sn->recvParsingStat = SNODE_RECV_STAT_PARSING_ARGV_NUM;
    }


PARSING_ARGV_START:

    if (0 == sn->argcRemaining) {
        sn->recvParsingStat = SNODE_RECV_STAT_PARSING_FINISHED;
        sn->recvStat = SNODE_RECV_STAT_PARSED;
        return;
    }

    if (0 == sdslen(sn->querybuf)) return;

//PARSING_ARGV_NUM:
    if (SNODE_RECV_STAT_PARSING_ARGV_NUM == sn->recvParsingStat) {
        sdsrange(sn->querybuf, 1, -1);//skip $
        readlen = parseInputBufferGetNum(sn, &(sn->tmpQuerybuf), &(sn->argvRemaining));

        if (EOF != readlen) return;

        if (sn->argvRemaining < 0) {
            setProtocolError(sn, 0);
            return;
        }

        sn->argvRemaining += 2; //增加读取 \r\n

        sdsclear(sn->tmpQuerybuf);
        sn->recvParsingStat = SNODE_RECV_STAT_PARSING_ARGV_VALUE;

    }

//PARSING_ARGV_VALUE:
    if (SNODE_RECV_STAT_PARSING_ARGV_VALUE == sn->recvParsingStat) {
        argJ = sn->argc - sn->argcRemaining;

        len = sdslen(sn->querybuf);

        /* querybuf少于需要读取的，读取所有querybuf，并返回 */
        if (sn->argvRemaining > len) {
            sn->argv[argJ] = sdscatlen(sn->argv[argJ], sn->querybuf, len);
            sdsclear(sn->querybuf);
            sn->argvRemaining -= len;
            return;
        }

        /* querybuf中有足够数据，则读取，并设置为读取完成 */
        sn->argv[argJ] = sdscatlen(sn->argv[argJ], sn->querybuf, sn->argvRemaining);

        if (len == sn->argvRemaining) {
            sdsclear(sn->querybuf);
        } else {
            sdsrange(sn->querybuf, sn->argvRemaining, -1);
        }


        sn->argvRemaining = 0;
        sn->recvParsingStat = SNODE_RECV_STAT_PARSING_ARGV_NUM;

        sn->argcRemaining--;

        sdsrange(sn->argv[argJ], 0, -3);//去掉 \r\n

        goto PARSING_ARGV_START;
    }

}

/* 解析读取数据
 */
static void parseInputBuffer(NTSnode *sn) {
    if (SNODE_RECV_STAT_ACCEPT == sn->recvStat || SNODE_RECV_STAT_PREPARE == sn->recvStat) {

        switch(sn->querybuf[0]) {
            case '+' :
                sn->recvType = SNODE_RECV_TYPE_OK;
                break;

            case '-' :
                sn->recvType = SNODE_RECV_TYPE_ERR;
                break;

            case '$' :
                sn->recvType = SNODE_RECV_TYPE_STRING;
                break;

            case '*' :
                sn->recvType = SNODE_RECV_TYPE_ARRAY;
                break;

            default :
                NTAddReplyError(sn, "Protocol error: Unrecognized type");
                setProtocolError(sn, 0);
                return;
        }

        sdsrange(sn->querybuf, 1, -1);
        sn->recvStat = SNODE_RECV_STAT_PARSING;
        sn->recvParsingStat = SNODE_RECV_STAT_PARSING_START;
    }

    if (SNODE_RECV_TYPE_OK == sn->recvType || SNODE_RECV_TYPE_ERR == sn->recvType) {
        parseInputBufferStatus(sn);

    } else if (SNODE_RECV_TYPE_STRING == sn->recvType) {
        parseInputBufferString(sn);

    } else if (SNODE_RECV_TYPE_ARRAY == sn->recvType) {
        parseInputBufferArray(sn);

        if (0 == sn->proc && sn->argc - sn->argcRemaining > 0) {
            dictEntry *de;

            de = dictFind(nt_server.services, sn->argv[0]);
            if (0 != de) {
                sn->proc = dictGetVal(de);
            }
        }

    } else {
        NTAddReplyError(sn, "Protocol error: Unrecognized");
        setProtocolError(sn, 0);
        return;
    }

    // 如果有在等待数据的回调函数，则在这里处理，并完成网络请求
    if (SNODE_RECV_STAT_PARSED == sn->recvStat) {
        if (0 != sn->responseProc) {
            sn->responseProc(sn);
            rePrepareNTSnodeToReadQuery(sn);
            return;
        }
    }

    // 如果没有等待数据的回调函数，则认为是一个新的命令
    if (0 == sn->proc) {
        if (SNODE_RECV_STAT_PARSED == sn->recvStat) {
            NTAddReplyError(sn, "command not found");
            rePrepareNTSnodeToReadQuery(sn);
        }
    } else {
        sn->proc(sn);

        if (SNODE_RECV_STAT_EXCUTED == sn->recvStat) {
            rePrepareNTSnodeToReadQuery(sn);
        }
    }
}

static void readQueryFromNTSnode(aeLooper *el, int fd, void *privdata, int mask) {
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

    nt_server.currentSnode = sn;

    nread = read(fd, sn->querybuf+sdslen(sn->querybuf), TRV_NET_IOBUF_LEN);
    if (nread >= 0) {
        sn->querybuf[sdslen(sn->querybuf)+nread] = 0;
        sdsupdatelen(sn->querybuf);
    }

    if (-1 == nread) {
        if (EAGAIN == errno) {
            nread = 0;
        }  else {
            TrvLogD("Reading from client: %s",strerror(errno));
            NTFreeNTSnode(sn);
        }
        return;
    } else if (0 == nread) {
        NTFreeNTSnode(sn);
        return;
    }

    while (sdslen(sn->querybuf) > 0) {
        parseInputBuffer(sn);
    }
}

static inline void freeScriptServiceRequestCtx(NTScriptServiceRequestCtx* ctx) {
    sdsfree(ctx->ScriptServiceCallbackUrl);
    sdsfree(ctx->ScriptServiceCallbackArg);
    zfree(ctx);
}

static inline void resetScriptServiceRequestCtx(NTScriptServiceRequestCtx *ctx) {
    sdsclear(ctx->ScriptServiceCallbackUrl);
    sdsclear(ctx->ScriptServiceCallbackArg);
    ctx->ScriptServiceLua = 0;
}

NTScriptServiceRequestCtx* NTNewScriptServiceRequestCtx() {
    NTScriptServiceRequestCtx *ctx;
    if (0 == listLength(nt_server.scriptServiceRequestCtxPool)) {
        for (int i = 0; i < 20; i++) {
            ctx = (NTScriptServiceRequestCtx*)zmalloc(sizeof(NTScriptServiceRequestCtx));
            memset(ctx, 0, sizeof(NTScriptServiceRequestCtx));
            ctx->ScriptServiceCallbackUrl = sdsempty();
            ctx->ScriptServiceCallbackArg = sdsempty();

            nt_server.scriptServiceRequestCtxPool = listAddNodeTail(nt_server.scriptServiceRequestCtxPool, ctx);
        }
    }

    listNode *ln = listFirst(nt_server.scriptServiceRequestCtxPool);
    ctx = (NTScriptServiceRequestCtx*)ln->value;
    listDelNode(nt_server.scriptServiceRequestCtxPool, ln);

    resetScriptServiceRequestCtx(ctx);

    return ctx;
}

void NTRecycleScriptServiceRequestCtx(void *_ctx) {
    NTScriptServiceRequestCtx *ctx = (NTScriptServiceRequestCtx*)_ctx;
    if (listLength(nt_server.scriptServiceRequestCtxPool) > 200) {
        listIter *li;
        listNode *ln;
        li = listGetIterator(nt_server.scriptServiceRequestCtxPool, AL_START_HEAD);
        for (int i = 0; i < 100 && NULL != (ln = listNext(li)); i++) {
            freeScriptServiceRequestCtx((NTScriptServiceRequestCtx*)listNodeValue(ln));
            listDelNode(nt_server.scriptServiceRequestCtxPool, ln);
        }
        listReleaseIterator(li);
    }

    nt_server.scriptServiceRequestCtxPool = listAddNodeTail(nt_server.scriptServiceRequestCtxPool, ctx);
}

static NTSnode* createNTSnode(int fd) {
    NTSnode *sn = zmalloc(sizeof(NTSnode));
    memset(sn, 0, sizeof(NTSnode));

    if (-1 == fd) {
        return 0;
    }

    anetNonBlock(0,fd);
    anetEnableTcpNoDelay(0,fd);
    if (nt_server.tcpkeepalive)
        anetKeepAlive(0,fd,nt_server.tcpkeepalive);
    if (aeCreateFileEvent(nt_el,fd,AE_READABLE,
                readQueryFromNTSnode, sn) == AE_ERR)
    {
        close(fd);
        zfree(sn);
        return 0;
    }

    sn->fd = fd;
    sprintf(sn->fdstr, "%d", sn->fd);
    sn->tmpQuerybuf = sdsempty();
    sn->querybuf = sdsempty();
    sn->writebuf = sdsempty();
    sn = snodeArgvEmpty(sn);
    sn->isWriteMod = 0;
    resetNTSnodeArgs(sn);

    sn->recvStat = SNODE_RECV_STAT_ACCEPT;
    
    sn->responseProc = 0;

    sn->scriptServiceRequestCtxList = listCreate();
    sn->scriptServiceRequestCtxList->free = NTRecycleScriptServiceRequestCtx;
    sn->scriptServiceRequestCtxListMaxId = 0;

    nt_server.statNumConnections++;
    dictAdd(nt_server.snodes, sn->fdstr, sn);

    return sn;
}

void NTFreeNTSnode(NTSnode *sn) {
    TrvLogD("Free NTSnode %d", sn->fd);

    if (-1 != sn->fd) {
        aeDeleteFileEvent(nt_el, sn->fd, AE_READABLE);
        aeDeleteFileEvent(nt_el, sn->fd, AE_WRITABLE);
        close(sn->fd);
    }

    sn = snodeArgvFree(sn);
    sdsfree(sn->tmpQuerybuf);
    sdsfree(sn->querybuf);
    sdsfree(sn->writebuf);

    dictDelete(nt_server.snodes, sn->fdstr);

    listRelease(sn->scriptServiceRequestCtxList);

    zfree(sn);

    nt_server.statNumConnections--;
}

NTSnode *NTConnectNTSnode(char *addr, int port) {
    int fd;
    NTSnode *sn;

    memset(nt_server.neterr, 0, ANET_ERR_LEN);
    fd = anetPeerSocket(nt_server.neterr, 0, "0.0.0.0", AF_INET);
    fd = anetPeerConnect(fd, nt_server.neterr, addr, port);
    //fd = anetPeerConnect(fd, nt_server.neterr, addr, port);
    if (ANET_ERR == fd) {
        TrvLogW("Unable to connect to %s", addr);
        return 0;
    }
    
    sn = createNTSnode(fd);
    return sn;
}

static void acceptCommonHandler(int fd, int flags) {
    NTSnode *sn;
    if (0 == (sn = createNTSnode(fd))) {
        TrvLogW("Error registering fd event for the new snode: %s (fd=%d)",
                strerror(errno),fd);
        close(fd); /* May be already closed, just ignore errors */
        return;
    }

    if (dictSize(nt_server.snodes) > nt_server.maxSnodes) {
        char *err = "-ERR max number of snodes reached\r\n";

        /* That's a best effort error message, don't check write errors */
        if (write(sn->fd,err,strlen(err)) == -1) {
            /* Nothing to do, Just to avoid the warning... */
        }
        nt_server.statRejectedConn++;
        NTFreeNTSnode(sn);
        return;
    }
    sn->flags |= flags;
}

static void acceptTcpHandler(aeLooper *el, int fd, void *privdata, int mask) {
    int cport, cfd, max = TRV_NET_MAX_ACCEPTS_PER_CALL;
    char cip[INET6_ADDRSTRLEN];
    NOTUSED(el);
    NOTUSED(mask);
    NOTUSED(privdata);

    while(max--) {
        cfd = anetTcpAccept(nt_server.neterr, fd, cip, sizeof(cip), &cport);
        if (cfd == ANET_ERR) {
            if (errno != EWOULDBLOCK)
                TrvLogW("Accepting snode connection: %s", nt_server.neterr);
            return;
        }
        acceptCommonHandler(cfd,0);
    }
}

static int listenToPort() {
    int loopJ; 

    nt_server.ipfdCount = 0;

    for (loopJ = 0; loopJ < nt_server.ipfdCount; loopJ++) {
        nt_server.ipfd[loopJ] = ANET_ERR;
    }

    if (nt_server.ipfd[0] != ANET_ERR) {
        anetPeerListen(nt_server.ipfd[0], nt_server.neterr, nt_server.tcpBacklog);
        nt_server.ipfdCount++;
    }

    if (nt_server.ipfd[1] != ANET_ERR) {
        anetPeerListen(nt_server.ipfd[1], nt_server.neterr, nt_server.tcpBacklog);
        nt_server.ipfdCount++;
    }

    if (0 == nt_server.ipfdCount) return ERRNO_ERR;

    for (loopJ = 0; loopJ < nt_server.ipfdCount; loopJ++) {
        if (aeCreateFileEvent(nt_el, nt_server.ipfd[loopJ], AE_READABLE,
                    acceptTcpHandler,0) == AE_ERR) {
            TrvLogE("Unrecoverable error creating nt_server.ipfd file event.");
        }
    }

    TrvLogI("监听端口: %d", nt_server.port);

    return ERRNO_OK;
}

NTSnode* NTGetNTSnodeByFDS(const char *fdstr) {
    dictEntry *de;

    de = dictFind(nt_server.snodes, fdstr);
    if (0 == de) {
        return 0;
    }

    return dictGetVal(de);
}

int NTPrepare(int listenPort) {
    nt_el = aeNewLooper(1024*1024);
    nt_server.unixtime = -1;
    nt_server.maxSnodes = TRV_NET_MAX_SNODE;

    nt_server.bindaddr = "0.0.0.0";
    nt_server.port = listenPort;
    nt_server.tcpBacklog = TRV_NET_TCP_BACKLOG;

    nt_server.statNumConnections = 0;
    nt_server.statRejectedConn = 0;

    nt_server.snodes = dictCreate(&stackStringTableDictType, 0);
    nt_server.snodeMaxQuerybufLen = SNODE_MAX_QUERYBUF_LEN;

    nt_server.tcpkeepalive = TRV_NET_TCPKEEPALIVE;

    nt_server.ipfd[0] = anetPeerSocket(nt_server.neterr, nt_server.port, nt_server.bindaddr, AF_INET);
    nt_server.ipfd[1] = anetPeerSocket(nt_server.neterr, nt_server.port, nt_server.bindaddr, AF_INET6);

    if (listenPort <= 0 || ERRNO_OK != listenToPort(nt_server.port, nt_server.ipfd, &nt_server.ipfdCount)) {
        TrvLogE("Listen to port err");
        return ERRNO_ERR;
    }

    nt_server.scriptServiceRequestCtxPool = listCreate();

    SVPrepare();

    return ERRNO_OK;
}
