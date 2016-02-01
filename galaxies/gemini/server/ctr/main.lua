local CtrMain = {}

function CtrMain.Index(connectId, argv)
    content = LoadView("index")

    NTAddReplyMultiString(connectId, nil, nil, content)
end

return CtrMain
