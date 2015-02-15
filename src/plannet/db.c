#include "core/util.h"
#include "core/zmalloc.h"
#include "plannet/plannet.h"
#include "plannet/db.h"

#include "lua.h"

#include "sqlite3.h"

extern char g_plannetdir[];
extern lua_State *g_plannetLuaSt;
extern sqlite3 *g_plannetDB;

void initPlannetDB() {
    int errno;
    char *filepath = zmalloc(ALLOW_PATH_SIZE);
    memset(filepath, 0, ALLOW_PATH_SIZE);

    snprintf(filepath, ALLOW_PATH_SIZE, "%s/sqlite.db", g_plannetdir);
    errno = sqlite3_open(filepath, &g_plannetDB);
    if (errno) {
        sqlite3_close(g_plannetDB);
        trvExit(0, "打开数据库失败，%s", filepath);
    }

    zfree(filepath);
}

int plannetDBQuery(lua_State *L) {
    const char *sql = lua_tostring(L, 1);
    char **dbresult;
    char *errmsg;
    int errno, nrow, ncolumn, loopI, loopJ, index;

    if (NULL == sql) return 0;

    errno = sqlite3_get_table(
            g_plannetDB,
            sql,
            &dbresult,
            &nrow,
            &ncolumn,
            &errmsg);

    if (SQLITE_OK != errno) {
        trvLogW("plannetDBQuery Error: %s", errmsg);
        sqlite3_free(errmsg);

        lua_pushnil(g_plannetLuaSt);
        return 1;
    }


    lua_newtable(g_plannetLuaSt);

    index = ncolumn;
    for (loopI = 0; loopI < nrow; loopI++) {
        lua_pushnumber(g_plannetLuaSt, loopI);

        lua_newtable(g_plannetLuaSt);

        for (loopJ = 0; loopJ < ncolumn; loopJ++) {
            lua_pushstring(g_plannetLuaSt, dbresult[loopJ]);
            lua_pushstring(g_plannetLuaSt, dbresult[index]);
            lua_settable(g_plannetLuaSt, -3);

            index++;
        }

        lua_settable(g_plannetLuaSt, -3);
    }

    sqlite3_free_table(dbresult);

    return 1;
}
