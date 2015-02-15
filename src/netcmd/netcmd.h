#ifndef __NETCMD_NETCMD_H
#define __NETCMD_NETCMD_H

void plannetCommand(struct Snode_s *sn); /* 发送给星球的命令 */
void testCommand(struct Snode_s *sn);
void closeCommand(struct Snode_s *sn);

void initNetCmd();

#endif
