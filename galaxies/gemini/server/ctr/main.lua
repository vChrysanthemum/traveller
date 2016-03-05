local CtrMain = {}

function CtrMain.Index(connectId, requestId, argv)
    content = core.util.LoadView("index")

    core.net.resp.ScriptServiceResponse(connectId, requestId, "data", content)
end

return CtrMain
