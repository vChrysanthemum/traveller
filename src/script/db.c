#include "core/util.h"
#include "core/zmalloc.h"
#include "script/script.h"

#include "lua.h"
#include "sqlite3.h"

// for lua
// 连接db
// @param filepath string
int STConnectDB(lua_State *L) {
    int errno;
    sqlite3 *db;

    const char *filepath = lua_tostring(L, 1);

    errno = sqlite3_open(filepath, &db);
    if (0 == errno) {
        lua_pushlightuserdata(L, db);
    } else {
        lua_pushnil(L);
        sqlite3_close(db);
    }

    return 1;
}

// for lua
// 关闭db连接
// @param db *db
int STCloseDB(lua_State *L) {
    int errno;
    sqlite3 *db;
    db = lua_touserdata(L, 1);
    TrvAssert((0 != db), "STCloseDB error");

    sqlite3_close(db);

    return 1;
}


// for lua
// 执行sql语句
// @param db  *db
// @param sql string
int STDBQuery(lua_State *L) {
    sqlite3 *db = lua_touserdata(L, 1);
    TrvAssert((0 != db), "STCloseDB error");

    const char *sql = lua_tostring(L, 2);
    char **dbresult;
    char *errmsg;
    int errno, nrow, ncolumn, loopI, loopJ, index;

    if (NULL == sql) return 0;

    errno = sqlite3_get_table(
            db,
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
