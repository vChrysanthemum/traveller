#include "core/util.h"
#include "core/zmalloc.h"
#include "script/planet.h"
#include "script/db.h"

#include "lua.h"

#include "sqlite3.h"

extern char g_planetdir[];
extern lua_State *g_planetLuaSt;
extern sqlite3 *g_planetDB;

void initPlannetDB() {
    int errno;
    char *filepath = zmalloc(ALLOW_PATH_SIZE);
    memset(filepath, 0, ALLOW_PATH_SIZE);

    snprintf(filepath, ALLOW_PATH_SIZE, "%s/sqlite.db", g_planetdir);
    errno = sqlite3_open(filepath, &g_planetDB);
    if (errno) {
        sqlite3_close(g_planetDB);
        trvExit(0, "打开数据库失败，%s", filepath);
    }

    zfree(filepath);
}

int planetDBQuery(lua_State *L) {
    const char *sql = lua_tostring(L, 1);
    char **dbresult;
    char *errmsg;
    int errno, nrow, ncolumn, loopI, loopJ, index;

    if (NULL == sql) return 0;

    errno = sqlite3_get_table(
            g_planetDB,
            sql,
            &dbresult,
            &nrow,
            &ncolumn,
            &errmsg);

    if (SQLITE_OK != errno) {
        trvLogW("planetDBQuery Error: %s", errmsg);
        sqlite3_free(errmsg);

        lua_pushnil(g_planetLuaSt);
        return 1;
    }


    lua_newtable(g_planetLuaSt);

    index = ncolumn;
    for (loopI = 0; loopI < nrow; loopI++) {
        lua_pushnumber(g_planetLuaSt, loopI);

        lua_newtable(g_planetLuaSt);

        for (loopJ = 0; loopJ < ncolumn; loopJ++) {
            lua_pushstring(g_planetLuaSt, dbresult[loopJ]);
            lua_pushstring(g_planetLuaSt, dbresult[index]);
            lua_settable(g_planetLuaSt, -3);

            index++;
        }

        lua_settable(g_planetLuaSt, -3);
    }

    sqlite3_free_table(dbresult);

    return 1;
}
