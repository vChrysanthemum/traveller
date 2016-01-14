#ifndef __NETCMD_NETCMD_H
#define __NETCMD_NETCMD_H

void galaxiesCommand(NTSnode *sn); /* 发送给星系的命令 */
void msgCommand(NTSnode *sn);
void testCommand(NTSnode *sn);
void closeCommand(NTSnode *sn);

void initNetCmd();

#endif
