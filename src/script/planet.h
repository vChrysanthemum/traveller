#ifndef __SCRIPT_PLANET_H
#define __SCRIPT_PLANET_H

#define PLANET_LUA_CALL_ERRNO_OK 0
#define PLANET_LUA_CALL_ERRNO_FUNC404 -1
#define PLANET_LUA_CALL_ERRNO_FUNC403 -2
#define PLANET_LUA_CALL_ERRNO_FUNC502 -3

#include "net/networking.h"

int STCallPlanetFunc(NTSnode *sn);
int STLoginPlanet(char *username, char *password);

#endif
