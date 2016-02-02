local CtrMain = {}

function CtrMain.Index(connectId, argv)
    content = LoadView("index")

    NTAddReplyMultiString(connectId, nil, nil, "scriptcbk", "data", content)
end

return CtrMain
