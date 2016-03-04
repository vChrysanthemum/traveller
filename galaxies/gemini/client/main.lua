package.path = package.path .. ';' .. g_basedir.."/?.lua;" .. g_basedir.."/../common/?.lua"

--[
-- g_basedir
-- g_conf
--] 

local g_serverContenctId = nil
local g_ServiceRouteTable = {}
local g_ServiceCallbackRouteTable = {}

function ServiceRouter(connectId, path, requestId, argv)
    if (g_ServiceRouteTable[path]) then
        g_ServiceRouteTable[path](connectId, requestId, argv)
    end
end

function ServiceCallbackRouter(connectId, cbkUrl, cbkArg, argv)
    if (g_ServiceCallbackRouteTable[cbkUrl]) then
        g_ServiceCallbackRouteTable[cbkUrl](connectId, cbkArg, argv)
    end
end

function Init(conf)
    g_conf = conf
    assert(nil ~= g_conf, "找不到配置: script:client")

    assert(nil ~= g_conf["galaxies_server_host"], "请配置 galaxies_server_host")
    assert(nil ~= g_conf["galaxies_server_port"], "请配置 galaxies_server_port")

    g_serverContenctId = core.nt.ConnectSnode(g_conf["galaxies_server_host"], g_conf["galaxies_server_port"])
    assert(nil ~= g_serverContenctId, "连接星系失败")

    core.util.LogI("连接星系成功")

    local CtrMain = require "ctr/main"
    local CtrCitizen = require "ctr/citizen"

    g_ServiceCallbackRouteTable["/index"] = CtrMain.CbkIndex
    g_ServiceCallbackRouteTable["/login"] = CtrCitizen.CbkLogin

    core.nt.ScriptServiceRequest(g_serverContenctId, "/index", "/index", nil)

    core.nt.ScriptServiceRequest(g_serverContenctId, "/login", "/login", nil,
    "email", "j@ioctl.cn", 
    "password", "123123")

    return
    
end
