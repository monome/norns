--- Log class
-- @module Log

require 'norns'


local LOG_MAX = 24

local Log = {}
Log.last = 1
Log.msg = {}
Log.time = {}
Log.len = 0

Log.post = function(txt)
    Log.msg[Log.last] = txt
    Log.time[Log.last] = sys.time()
    Log.last = (Log.last + 1) % LOG_MAX
    Log.len = Log.len + 1
end

Log.get = function(index)
    if index <= Log.len then
        local i = (Log.last - index) % LOG_MAX
        local m = (sys.time() - Log.time[i]) .. " " .. Log.msg[i] 
        return m 
    else
        return "..."
    end
end

return Log 
