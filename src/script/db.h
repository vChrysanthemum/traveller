#ifndef __SCRIPT_DB_H
#define __SCRIPT_DB_H

#include "lua.h"
#include "sqlite3.h"

sqlite3* STInitDB(char *filepath);
int STDBQuery(lua_State *L);

#endif
