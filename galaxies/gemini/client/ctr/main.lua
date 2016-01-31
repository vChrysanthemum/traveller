local CtrMain = {}

function CtrMain.CbkIndex(connectId, arg, netRecvType, content)
    UILoadPage(content)
end

return CtrMain
