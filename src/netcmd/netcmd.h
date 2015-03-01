#ifndef __NETCMD_NETCMD_H
#define __NETCMD_NETCMD_H

void planetCommand(struct NTSnode_s *sn); /* 发送给星球的命令 */
void msgCommand(struct NTSnode_s *sn);
void testCommand(struct NTSnode_s *sn);
void closeCommand(struct NTSnode_s *sn);

void initNetCmd();

int NTPrepareBlockCmd(NTSnode *sn);
void NTBlockCmd(NTSnode *sn);
void NTFinishBlockCmd(NTSnode *sn);
void NTAwakeBlockCmd(NTSnode *sn);

#endif
