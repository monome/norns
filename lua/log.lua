--- Log class
-- @module Log

require 'norns'


local LOG_MAX = 24

local Log = {}
Log.last = 1
Log.msg = {}
Log.time = {}
Log.length = 0

Log.post = function(txt)
    Log.msg[Log.last] = txt
    Log.time[Log.last] = sys.time()
    Log.last = (Log.last + 1) % LOG_MAX
    Log.length = clamp(Log.length + 1, 0, 24)
end

Log.get = function(index)
    if index <= Log.length then
        local i = (Log.last - index) % LOG_MAX
        local m = (sys.time() - Log.time[i]) .. " " .. Log.msg[i] 
        return m 
    else
        return "..."
    end
end

Log.len = function()
    return Log.length
end

return Log 
