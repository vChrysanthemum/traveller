local util = require "util"
local ini = {}

function ini.Parse(inistr)
	local tab = {}
	local line = ""
	local newLine
	local i
	local currentTag = nil
	local found = false
    local pos = 0
    for line in string.gmatch(inistr, "[^\n]+\n") do
		found = false		
		line = line:gsub("\\;", "#_!36!_#") -- to keep \;
		line = line:gsub("\\=", "#_!71!_#") -- to keep \=
		if line ~= "" then
			-- Delete comments
			newLine = line
			line = ""
			for i=1, string.len(newLine) do				
				if string.sub(newLine, i, i) ~= ";" then
					line = line..newLine:sub(i, i)						
				else				
					break
				end
			end
			line = util.StringTrim(line)
			-- Find tag			
			if line:sub(1, 1) == "[" and line:sub(line:len(), line:len()) == "]" then
				currentTag = util.StringTrim(line:sub(2, line:len()-1))
				tab[currentTag] = {}
				found = true							
			end
			-- Find key and values
			if not found and line ~= "" then				
				pos = line:find("=")
				if pos == nil then
					goto ParseNewLine
                end
				line = line:gsub("#_!36!_#", ";")
				line = line:gsub("#_!71!_#", "=")
				tab[currentTag][util.StringTrim(line:sub(1, pos-1))] = util.StringTrim(line:sub(pos+1, line:len()))
				found = true			
			end			
        end
        ::ParseNewLine::
    end
    return tab
end

function ini.LoadConf(confStrList, confKey)
    local conf = {}
    for k, confstr in pairs(confStrList) do 
        tab = ini.Parse(confstr)
        for k2, tabv in pairs(tab) do 
            if confKey == k2 then
                conf = util.TableMerge(conf, tabv)
            end
        end
    end

    return conf
end

return ini
