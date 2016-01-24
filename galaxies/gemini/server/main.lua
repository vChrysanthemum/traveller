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

    g_ServiceRouteTable["/citizen/login"] = CtrCitizenLogin
end

function GalaxiesRouter(connectId, index, ...)
    CtrCitizenLogin(connectId, ...)
end

function ServiceRouter(connectId, index, ...)
end

function SrvCallbackRouter(connectId, index, ...)
end
