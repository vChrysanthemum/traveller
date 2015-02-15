package.path = package.path .. ';' .. basedir.."/?.lua;"

function init()
    loggedCitizens = {}

    util = require("util")
    md5 = require("md5")
    citizen = require("citizen")

    print("开始初始化月球")
end
