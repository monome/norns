local LOG_MAX = 24

norns.log = {}
norns.log.last = 1
norns.log.msg = {}
norns.log.time = {}
norns.log.len = 0

norns.log.post = function(txt)
    norns.log.msg[norns.log.last] = txt
    norns.log.time[norns.log.last] = norns.time()
    norns.log.last = (norns.log.last + 1) % LOG_MAX
    norns.log.len = norns.log.len + 1
end

norns.log.get = function(index)
    if index <= norns.log.len then
        local i = (norns.log.last - index) % LOG_MAX
        local m = (norns.time() - norns.log.time[i]) .. " " .. norns.log.msg[i] 
        return m 
    else
        return "..."
    end
end
