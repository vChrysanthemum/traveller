local util = {}

function util.StringTrim(s)
	assert(s ~= nil, "String can't be nil")
    return (string.gsub(s, "^%s*(.-)%s*$", "%1"))
end

function util.TableMerge(table1, table2)
    local found = false
    for k2, v2 in pairs(table2) do

        found = false
        for k1, v1 in pairs(table1) do
            if k1 == k2 then 
                table1[k1] = v2 
                found = true
            end
        end

        if false == found then
            table1[k2] = v2
        end
    end

    return table1
end

return util
