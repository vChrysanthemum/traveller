local db = {
    instance = nil,
    dbpath = nil
}

function db:SetConf(dbpath)
    self.dbpath = dbpath
end

function db:Instance()
    if nil ~= self.instance then
        return self.instance
    end

    local obj = {
        conn = nil,
        dbpath = self.dbpath
    }
    self.__index = self
    obj.__gc = function(p)
        p:Close()
    end
    obj.conn = DB_Connect(obj.dbpath)
    assert(nil ~= obj.conn, "连接数据库失败")
    self.instance = setmetatable(obj, self)

    return self.instance
end

function db:escape(str)
    return "'" .. string.gsub(str, "'", "''") .. "'"
end

function db:close()
    DB_Close(self.conn)
end

function db:Query(sql)
    return DB_Query(self.conn, sql)
end

function db:Create(tablename, data)
    local sql = "INSERT INTO " .. tablename .. " "
    local pairs = pairs
    local columns = {}
    local values = {}
    for k, v in pairs(data) do
        table.insert(columns, k)
        table.insert(values, self:escape(v))
    end
    columns = table.concat(columns, ",")
    values = table.concat(values, ",")

    sql = sql .. "(".. columns ..") " .. "VALUES (".. values ..");"
    return self:Query(sql)
end

function db:Update(tablename, data, where)
    local sql = "UPDATE " .. tablename .. " "
    local pairs = pairs

    local sql_data= {}
    local sql_where = {}

    for k, v in pairs(data) do
        table.insert(sql_data, k.."='"..v.."'")
    end
    sql_data = table.concat(sql_data, ",")

    for k, v in pairs(where) do
        table.insert(sql_where, k.."='"..v.."'")
    end
    sql_where = table.concat(sql_where, " AND ")

    sql = sql .. " SET " .. sql_data .. " WHERE " .. sql_where
    return self:Query(sql)
end

function db:Delete(tablename, where)
    local sql = "DELETE FROM " .. tablename .. " "
    local sql_where = {}
    for k, v in pairs(where) do
        table.insert(sql_where, k.."='"..v.."'")
    end
    sql_where = table.concat(sql_where, " AND ")

    sql = sql .. " WHERE " .. sql_where
    return self:Query(sql)
end

function db:Select(tablename, columns, where, orderby, limit)
    local sql = "SELECT " .. columns .. " FROM " .. tablename .. " "
    local sql_where = {}
    for k, v in pairs(where) do
        table.insert(sql_where, k.."='"..v.."'")
    end
    sql_where = table.concat(sql_where, " AND ")


    local sql_orderby
    if orderby then
        sql_orderby = " ORDER BY " .. orderby .. " "
    else 
        sql_orderby = ""
    end

    local sql_limit
    if limit then 
        sql_limit = "LIMIT " .. sql_limit
    else 
        sql_limit = ""
    end


    sql = sql .. " WHERE " .. sql_where .. sql_orderby .. sql_limit
    return self:Query(sql)
end

return db
