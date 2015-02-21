function test_db()
    local pairs = pairs
    local ret = dbQuery("select * from b_test order by test_id asc limit 5")
    local ret_count = #ret
    if ret then
        for k = 0, ret_count, 1 do
            print(ret[k]['test_id'])
            print(ret[k]['data'])
            print(ret[k]['d'])
            print('')
        end
    elseif nil == ret then
        print(nil)
    end
end
