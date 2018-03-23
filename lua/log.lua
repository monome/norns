--- Log class
-- @module log

local LOG_MAX = 24

local Log = {}
Log.last = 1
Log.msg = {}
Log.time = {}
Log.length = 0

--- post message to log, timestamped
-- @param txt log message
Log.post = function(txt)
    Log.msg[Log.last] = txt
    Log.time[Log.last] = sys.time()
    Log.last = (Log.last + 1) % LOG_MAX
    Log.length = clamp(Log.length + 1, 0, 24)
end

--- get log message at index, with timestamp (seconds old)
-- @param index which message
Log.get = function(index)
    if index <= Log.length then
        local i = (Log.last - index) % LOG_MAX
        local m = (sys.time() - Log.time[i]) .. " " .. Log.msg[i] 
        return m 
    else
        return "..."
    end
end

--- get log length
Log.len = function()
    return Log.length
end

return Log 
