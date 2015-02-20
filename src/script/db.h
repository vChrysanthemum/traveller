#ifndef __PLANNET_DB_H
#define __PLANNET_DB_H

#include "lua.h"

void initPlannetDB();
int planetDBQuery(lua_State *L);

#endif
