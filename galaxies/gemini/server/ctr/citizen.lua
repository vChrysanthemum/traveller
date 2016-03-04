local CtrCitizen = {}

local g_db = nil
local md5 = require "md5"
local db = require "db"

function CtrCitizen.Login(connectId, requestId, argv)
    core.util.LogI("hi")
    core.util.LogI(connectId.. " connectId")
    core.util.LogI(argv["email"].." email")
    core.util.LogI(argv["password"].." password")

    g_db = db:Instance()

    if nil == email or nil == password then
        core.nt.ScriptServiceResponse(connectId, requestId, "err", "请输入用户名或密码")
        return
    end

    local citizen = g_db:Select(
    'b_citizen', 
    'citizen_id,code,nickname,email', 
    {email=email, password=md5.sumhexa(password)})

    if not citizen[0] then
        core.nt.ScriptServiceResponse(connectId, requestId, "err", "用户名或密码错误哈哈哈")
        return
    end
    citizen = citizen[0]

    g_loggedCitizens[connectId] = citizen
    core.nt.ScriptServiceResponse(connectId, requestId, "msg", "登陆成功")

end

return CtrCitizen
