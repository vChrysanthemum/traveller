local CtrMain = {}

function CtrMain.Index(connectId, requestId, argv)
    content = core.util.LoadView("index")

    core.nt.ScriptServiceResponse(connectId, requestId, "data", content)
end

return CtrMain
