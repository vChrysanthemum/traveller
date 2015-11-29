#ifndef __NETCMD_NETCMD_H
#define __NETCMD_NETCMD_H

void galaxiesCommand(struct NTSnode_s *sn); /* 发送给星系的命令 */
void msgCommand(struct NTSnode_s *sn);
void testCommand(struct NTSnode_s *sn);
void closeCommand(struct NTSnode_s *sn);

void initNetCmd();

#endif
