local CtrMain = {}

function CtrMain.CbkIndex(connectId, cbkArg, argv)
    UILoadPage(argv["data"])
end

return CtrMain
