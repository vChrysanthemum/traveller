local CtrMain = {}

function CtrMain.CbkIndex(connectId, arg, netRecvType, argv)
    UILoadPage(argv["data"])
end

return CtrMain
