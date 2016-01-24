#ifndef __SCRIPT_GALAXIES_H
#define __SCRIPT_GALAXIES_H

#define GALAXIES_LUA_CALL_ERRNO_OK 0
#define GALAXIES_LUA_CALL_ERRNO_FUNC502 -3

#include "net/networking.h"

/* 调用星系上的函数
 * 只允许访问函数名 PUB 为开头的函数
 * lua中函数只返回一个字符串
 * argv 为 sn->argv[1:]，既忽略 sn的procName
 */
int STCallGalaxyFunc(NTSnode *sn);

/* 玩家登录星系
 */
int STLoginGalaxy(char *username, char *password);

#endif
