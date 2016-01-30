package.path = package.path .. ';' .. g_basedir.."/?.lua;" .. g_basedir.."/../common/?.lua"

--[
-- g_basedir
-- g_conf
--] 

local g_serverContenctId = nil
local g_ServiceRouteTable = {}
local g_SrvCallbackRouteTable = {}

function CbkLogin(connectId, arg, netRecvType, ...)
    LogI("fuck logined")
    LogI(arg)
    LogI(netRecvType)
    LogI(...)
    a = LoadView("main")
    LogI(a)
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

function Init(conf)
    g_conf = conf
    assert(nil ~= g_conf, "找不到配置: script:client")

    LogI(g_conf)
    assert(nil ~= g_conf["galaxies_server_host"], "请配置 galaxies_server_host")
    assert(nil ~= g_conf["galaxies_server_port"], "请配置 galaxies_server_port")

    g_serverContenctId = NTConnectNTSnode(g_conf["galaxies_server_host"], g_conf["galaxies_server_port"])
    assert(nil ~= g_serverContenctId, "连接星系失败")

    LogI("连接星系成功")

    g_SrvCallbackRouteTable["/login"] = CbkLogin

    NTAddReplyMultiString(g_serverContenctId, "/login", nil, 
    "script", "/login", "j@ioctl", "fuckemail", "fuckpassword")
end
