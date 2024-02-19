--- hotswap library

--- globals are available on crow, otherwise require for norns
local require = require or function() end
local s = sequins or require 'lib/sequins'
local tl = timeline or require 'lib/timeline'

local HS = {
    _reg = {} -- a place to register updateable sequins
}

HS.cleanup = function()
    HS._reg = {}
end

-- to add support for a new type you need: 
-- 1) add an elseif predicate for capturing the type
-- 2) give the type a single character identifier
-- 3) add a swap function to HS._swap table

HS._type = function(o)
    -- return a char representing the type
    if s.is_sequins(o) then return 's'
    elseif tl.is_timeline(o) then return 't'
    else print 'hotswap does not know this type'
    end
end

HS._swap = {
    s = function(t, v) t:settable(v) end,
    t = function(t, v) t:hotswap(v) end,
}


HS.__index = function(self, ix)
    return HS._reg[ix][2]
end

HS.__newindex = function(self, ix, v)
    local t = HS._type(v)
    if t then
        if HS._reg[ix] then -- key already exists
            -- warning! we assume the new type matches
            HS._swap[t](HS._reg[ix][2], v)
        else -- new key
            HS._reg[ix] = {t,v} -- register with type annotation
        end
    end
end

return setmetatable(HS, HS)
