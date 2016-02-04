local CtrMain = {}

function CtrMain.Index(connectId, requestId, argv)
    content = LoadView("index")

    NTScriptServiceResponse(connectId, requestId, "data", content)
end

return CtrMain
