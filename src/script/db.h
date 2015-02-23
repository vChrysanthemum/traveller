#ifndef __SCRIPT_DB_H
#define __SCRIPT_DB_H

#include "lua.h"

void STInitDB();
int STDBQuery(lua_State *L);

#endif
