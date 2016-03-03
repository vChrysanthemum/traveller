local CtrMain = {}

function CtrMain.Index(connectId, requestId, argv)
    content = LoadView("index")

    NT_ScriptServiceResponse(connectId, requestId, "data", content)
end

return CtrMain
