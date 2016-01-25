package.path = package.path .. ';' .. g_basedir.."/?.lua;"

--[[
--global
--  g_basedir
--]]
--
local g_ServiceRouteTable = {}
local loggedCitizens = {}

function Init()
    util = require("util")
    md5 = require("md5")
    citizen = require("citizen")

    g_ServiceRouteTable["/login"] = CtrLogin
end

function ServiceRouter(connectId, path, ...)
    LogI(path)
    if (g_ServiceRouteTable[path]) then
        g_ServiceRouteTable[path](connectId, ...)
    end
end

function SrvCallbackRouter(connectId, path, ...)
end
