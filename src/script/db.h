#ifndef __PLANNET_DB_H
#define __PLANNET_DB_H

#include "lua.h"

void STInitDB();
int STDBQuery(lua_State *L);

#endif
