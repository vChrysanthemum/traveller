local util = {
    db = {fuck = 321}
}

function util.db:query(sql)
    return DBQuery(sql)
end

function util.db:escape(str)
    return "'" .. string.gsub(str, "'", "''") .. "'"
end

function util.db:create(tablename, data)
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
    return self:query(sql)
end

function util.db:update(tablename, data, where)
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
    return self:query(sql)
end


function util.db:delete(tablename, where)
    local sql = "DELETE FROM " .. tablename .. " "
    local sql_where = {}
    for k, v in pairs(where) do
        table.insert(sql_where, k.."='"..v.."'")
    end
    sql_where = table.concat(sql_where, " AND ")

    sql = sql .. " WHERE " .. sql_where
    return self:query(sql)
end


function util.db:select(tablename, columns, where, orderby, limit)
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
    return self:query(sql)
end

return util
