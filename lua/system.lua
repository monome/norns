--- system module;
-- this stuff should be sorted and moved 
-- @module system

--- execute os command, capture output
-- @param cmd command
-- @param raw raw output (omit for scrubbed)
-- @return ouput
function os.capture(cmd, raw)
    local f = assert(io.popen(cmd, 'r'))
    local s = assert(f:read('*a'))
    f:close()
    if raw then return s end
    s = string.gsub(s, '^%s+', '')
    s = string.gsub(s, '%s+$', '')
    s = string.gsub(s, '[\n\r]+', ' ')
    return s
end

--- string begins with
-- @param s string to examine
-- @param start string to search for
-- @return true or false
function string.starts(s,start)
    return string.sub(s,1,string.len(start))==start
end

--- tablelength FIXME (redundant)
function tablelength(T)
	local count = 0
	for _ in pairs(T) do count = count + 1 end
	return count
end
