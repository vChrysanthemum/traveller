#ifndef __PLANNET_PLANNET_H
#define __PLANNET_PLANNET_H

#define PLANNET_LUA_CALL_ERRNO_OK 0
#define PLANNET_LUA_CALL_ERRNO_FUNC404 -1
#define PLANNET_LUA_CALL_ERRNO_FUNC403 -2
#define PLANNET_LUA_CALL_ERRNO_FUNC502 -3

#include "net/networking.h"

void STInitPlanet();
int STCallPlanetFunc(NTSnode *sn);

#endif
