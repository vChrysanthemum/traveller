package.path = package.path .. ';' .. g_basedir.."/?.lua;" .. g_basedir.."/../common/?.lua"

--[
-- g_basedir
-- g_conf
--] 

local g_serverContenctId = nil
local g_ServiceRouteTable = {}
local g_ServiceCallbackRouteTable = {}

function CbkLogin(connectId, scriptArg, netRecvType, ...)
    LogI("fuck logined")
    LogI(scriptArg)
    LogI(netRecvType)
    LogI(...)
end

function ServiceRouter(connectId, path, ...)
    if (g_ServiceRouteTable[path]) then
        g_ServiceRouteTable[path](connectId, ...)
    end
end

function ServiceCallbackRouter(connectId, path, ...)
    if (g_ServiceCallbackRouteTable[path]) then
        g_ServiceCallbackRouteTable[path](connectId, ...)
    end
end

function Init(conf)
    g_conf = conf
    assert(nil ~= g_conf, "找不到配置: script:client")

    assert(nil ~= g_conf["galaxies_server_host"], "请配置 galaxies_server_host")
    assert(nil ~= g_conf["galaxies_server_port"], "请配置 galaxies_server_port")

    g_serverContenctId = NTConnectNTSnode(g_conf["galaxies_server_host"], g_conf["galaxies_server_port"])
    assert(nil ~= g_serverContenctId, "连接星系失败")

    LogI("连接星系成功")

    local CtrMain = require "ctr/main"

    g_ServiceCallbackRouteTable["/index"] = CtrMain.CbkIndex
    g_ServiceCallbackRouteTable["/login"] = CbkLogin

    NTAddReplyMultiString(g_serverContenctId, "/index", nil, "script", "/index")

    --[
    --NTAddReplyMultiString(g_serverContenctId, "/login", nil, "script", "/login", 
    --"email", "j@ioctl", 
    --"password", "fuckpassword")
    --]

    return
    
end
