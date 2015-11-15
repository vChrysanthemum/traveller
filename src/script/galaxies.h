#ifndef __SCRIPT_GALAXIES_H
#define __SCRIPT_GALAXIES_H

#define GALAXIES_LUA_CALL_ERRNO_OK 0
#define GALAXIES_LUA_CALL_ERRNO_FUNC404 -1
#define GALAXIES_LUA_CALL_ERRNO_FUNC403 -2
#define GALAXIES_LUA_CALL_ERRNO_FUNC502 -3

#include "net/networking.h"

int STCallPlanetFunc(NTSnode *sn);
int STLoginPlanet(char *username, char *password);

#endif
