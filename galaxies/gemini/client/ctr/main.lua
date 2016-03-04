local CtrMain = {}

function CtrMain.CbkIndex(connectId, cbkArg, argv)
    core.ui.LoadPage(argv["data"])
end

return CtrMain
