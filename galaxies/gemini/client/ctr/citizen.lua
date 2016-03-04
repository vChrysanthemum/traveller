local CtrCitizen = {}

function CtrCitizen.CbkLogin(connectId, cbkArg, argv)
    core.util.LogI("fuck logined")
    core.util.LogI(argv["err"])
    core.util.LogI(argv["msg"])
end

return CtrCitizen
