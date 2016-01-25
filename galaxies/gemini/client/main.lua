package.path = package.path .. ';' .. g_basedir.."/?.lua;"

--[[
--global
--  g_basedir
--  g_serverContenctId
--]]

local g_ServiceRouteTable = {}
local g_SrvCallbackRouteTable = {}

function CtrIndex(contectId, ...)
    LogI("/login/"..contectId)
    NTAddReplyMultiString(g_serverContenctId, "/login", nil, "galaxies", "j@ioctl", "fuckemail", "fuckpassword")
end

function CbkLogin(contectId, arg, netRecvType, ...)
    LogI("fuck logined")
    LogI(arg)
    LogI(netRecvType)
    LogI(...)
end

function ServiceRouter(contectId, path, ...)
    if (g_ServiceRouteTable[path]) then
        g_ServiceRouteTable[path](contectId, ...)
    end
end

function SrvCallbackRouter(contectId, path, ...)
    if (g_SrvCallbackRouteTable[path]) then
        g_SrvCallbackRouteTable[path](contectId, ...)
    end
end

function Init()
    g_ServiceRouteTable["/index"] = CtrIndex

    g_SrvCallbackRouteTable["/login"] = CbkLogin
end
