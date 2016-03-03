local CtrCitizen = {}

function CtrCitizen.CbkLogin(connectId, cbkArg, argv)
    LogI("fuck logined")
    LogI(argv["err"])
    LogI(argv["msg"])
end

return CtrCitizen
