local CtrMain = {}

function CtrMain.Index(connectId)
    content = LoadView("index")

    NTAddReplyMultiString(connectId, nil, nil, content)
end

return CtrMain
