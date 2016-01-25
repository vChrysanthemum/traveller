#include "core/util.h"
#include "core/zmalloc.h"
#include "script/script.h"

#include "lua.h"
#include "sqlite3.h"

extern sqlite3 *g_srvDB;

sqlite3* STinitDB(char *filepath) {
    int errno;
    sqlite3 *db;

    errno = sqlite3_open(filepath, &db);
    if (errno) {
        sqlite3_close(db);
        TrvExit(0, "打开数据库失败，%s", filepath);
    }

    return db;
}

int STDBQuery(lua_State *L) {
    const char *sql = lua_tostring(L, 1);
    char **dbresult;
    char *errmsg;
    int errno, nrow, ncolumn, loopI, loopJ, index;

    if (NULL == sql) return 0;

    errno = sqlite3_get_table(
            g_srvDB,
            sql,
            &dbresult,
            &nrow,
            &ncolumn,
            &errmsg);

    if (SQLITE_OK != errno) {
        TrvLogW("STDBQuery Error: %s", errmsg);
        sqlite3_free(errmsg);

        lua_pushnil(L);
        return 1;
    }


    lua_newtable(L);

    index = ncolumn;
    for (loopI = 0; loopI < nrow; loopI++) {
        lua_pushnumber(L, loopI);

        lua_newtable(L);

        for (loopJ = 0; loopJ < ncolumn; loopJ++) {
            lua_pushstring(L, dbresult[loopJ]);
            lua_pushstring(L, dbresult[index]);
            lua_settable(L, -3);

            index++;
        }

        lua_settable(L, -3);
    }

    sqlite3_free_table(dbresult);

    return 1;
}
