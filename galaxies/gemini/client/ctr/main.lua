local CtrMain = {}

function CtrMain.CbkIndex(connectId, cbkArg, argv)
    UI_LoadPage(argv["data"])
end

return CtrMain
