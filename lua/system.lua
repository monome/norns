--- system module
-- @module system

--- power management
-- @section power

--- redefine battery percent handler
-- @param percent battery full percentage
norns.battery = function(percent)
    norns.batterypercent = tonumber(percent)
    --print("battery: "..norns.batterypercent.."%")
end

--- redefine power present handler
-- @param present power plug present (0=no,1=yes)
norns.power = function(present)
    norns.powerpresent = present
    print("power: "..present)
end

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

function string.starts(s,start)
    return string.sub(s,1,string.len(start))==start
end

function tablelength(T)
	local count = 0
	for _ in pairs(T) do count = count + 1 end
	return count
end
