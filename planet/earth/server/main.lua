package.path = package.path .. ';' .. basedir.."/?.lua;"

function init()
    loggedCitizens = {}

    util = require("util")
    md5 = require("md5")
    citizen = require("citizen")
end
