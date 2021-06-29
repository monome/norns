--- Quote sub-library
-- fns for stringifying data-structures
-- output is a string that can be read back as lua code with call()

local Q = {
  OPTIMIZE_LENGTH = false -- true reduces tx-length of strings, but takes longer to calc
}


function Q.key(ix)
    -- stringify a table key with explicit [] style
    local fstr = (type(ix)=='number') and '[%g]' or '[%q]'
    return string.format(fstr, ix)
end

function Q.quote(val, ...)
    -- stringify any data so lua can load() it
    if ... ~= nil then
        local t = {Q.quote(val)} -- capture 1st arg
        for _,v in ipairs{...} do -- capture varargs
            table.insert(t, Q.quote(v))
        end
        return table.concat(t, ',')
    elseif type(val) == 'string' then return string.format('%q',val)
    elseif type(val) == 'number' then return string.format('%.6g',val) -- 6 sig figures
    elseif type(val) ~= 'table' then return tostring(val)
    else -- recur per table element
        local t = {}
        if Q.OPTIMIZE_LENGTH then
            local max = 0
            -- add array-style keys for reduced string length
            for k,v in ipairs(val) do
                table.insert(t, Q.quote(v))
                max = k -- save highest ipair key
            end
            for k,v in pairs(val) do
                -- match on any key that wasn't caught by ipairs (without needing to copy the table)
                    -- not a number, is a float, is a sparse int key, is a zero or less int key
                if type(k) ~= 'number' or k ~= math.floor(k) or k > max or k < 1 then
                    table.insert(t, Q.key(k) .. '=' .. Q.quote(v))
                end
            end
        else -- faster to build, but transmission is longer
            for k,v in pairs(val) do
                table.insert(t, Q.key(k) .. '=' .. Q.quote(v))
            end
        end
        return string.format('{%s}', table.concat(t, ','))
    end
end

setmetatable(Q,{
  __call = function(self, ...) return Q.quote(...) end
})

return Q

