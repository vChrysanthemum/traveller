package.path = package.path .. ';' .. g_basedir.."/?.lua;"

--[[
--global
--  g_basedir
--  g_serverContenctId
--]]

local g_ServiceRouteTable = {}
local g_SrvCallbackRouteTable = {}

function CbkLogin(connectId, arg, netRecvType, ...)
    LogI("fuck logined")
    LogI(arg)
    LogI(netRecvType)
    LogI(...)
end

function ServiceRouter(connectId, path, ...)
    if (g_ServiceRouteTable[path]) then
        g_ServiceRouteTable[path](connectId, ...)
    end
end

function SrvCallbackRouter(connectId, path, ...)
    if (g_SrvCallbackRouteTable[path]) then
        g_SrvCallbackRouteTable[path](connectId, ...)
    end
end

function Init()
    g_SrvCallbackRouteTable["/login"] = CbkLogin

    NTAddReplyMultiString(g_serverContenctId, "/login", nil, 
    "galaxies", "/login", "j@ioctl", "fuckemail", "fuckpassword")
end
