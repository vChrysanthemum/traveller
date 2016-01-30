package.path = package.path .. ';' .. g_basedir.."/?.lua;" .. g_basedir.."/../common/?.lua;" .. g_basedir.."/ctr/?.lua"

--[
-- g_basedir
-- g_conf
--] 

local g_db
local g_ServiceRouteTable = {}

local md5 = require "md5"
local db = require "db"

function ServiceRouter(connectId, path, ...)
    if (g_ServiceRouteTable[path]) then
        g_ServiceRouteTable[path](connectId, ...)
    end
end

function SrvCallbackRouter(connectId, path, ...)
end

function Init(conf)
    g_conf = conf
    assert(nil ~= g_conf, "找不到配置: script:server")

    db:SetConf(g_basedir.."/sqlite.db")
    g_db = db:Instance()
    assert(nil ~= g_db, "连接数据库失败")

    local CtrCitizen = require "ctr/citizen"
    g_ServiceRouteTable["/login"] = CtrCitizen.Login
end
