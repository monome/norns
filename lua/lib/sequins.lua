--- sequins

local S = {}

-- convert a string to a table of chars
local function totable(t)
    if type(t) == 'string' then
        local tmp = {}
        t:gsub('.', function(c) table.insert(tmp,c) end)
        return tmp
    end
    return t
end

function S.new(t)
    t = totable(t) -- convert a string to a table of chars
    -- wrap a table in a sequins with defaults
    local s = { data   = t
              , length = #t -- memoize for efficiency
              , ix     = 1
              , qix    = 1 -- force 1st value to 1st step
              , n      = 1 -- store the step val (can be a sequins)
              , flw    = {} -- store any applied flow modifiers
              , fun    = {} -- store a transformer function & any additional arguments
              }
    return setmetatable(s, S)
end

local function wrap_index(s, ix) return ((ix - 1) % s.length) + 1 end

function S:is_sequins() return getmetatable(self) == S end

function S:setdata(t)
    if S.is_sequins(t) then
        do -- flow modifiers
            local src = t.flw
            local dst = self.flw

            -- update / remove existing
            for k,v in pairs(dst) do
                if src[k] then -- already exists
                    dst[k].n  = src[k].n -- update value
                else dst[k] = nil end -- removed
            end

            -- add any new
            for k,v in pairs(src) do
                if not dst[k] then -- newly added
                    dst[k] = {ix = v.ix, n = v.n} -- manual copy
                end
            end
        end

        do -- update data of transformers
            local src = t.fun
            local dst = self.fun

            if dst[1] then -- already have a transformer
                dst[1] = src[1] -- copy function
                if src[2] then
                    for k,v in ipairs(src[2]) do
                         if S.is_sequins(v) and dst[2] and S.is_sequins(dst[2][k]) then
                            dst[2][k]:settable(v) -- recurse nested sequins
                        else
                            dst[2][k] = v -- copy piecemeal
                        end
                    end
                end
            elseif src[1] then -- no existing transformer, so we just pull in the new one
                print'new transformer'
                self.fun = t.fun
            end
        end

        -- finally, place data over top of input sequins
        t = t.data -- handle sequins data as input table
    end

    t = totable(t) -- convert a string to a table of chars

    -- data swap
    for i=1,#t do
        if S.is_sequins(t[i]) and S.is_sequins(self.data[i]) then
            self.data[i]:settable(t[i]) -- recurse nested sequins
        else
            self.data[i] = t[i] -- copy table piecemeal
        end
    end
    self.data[#t+1] = nil -- disregard any surplus data

    self.length = #t
    self.ix = wrap_index(self, self.ix)
end

function S:copy()
    if type(self) == 'table' then
        local cp = {}
        for k,v in pairs(self) do
            cp[k] = S.copy(v)
        end
        return setmetatable(cp, getmetatable(self))
    else return self end
end

function S:peek() return self.data[self.ix] end

function S:bake(n)
    n = n or #self -- void call creates a table of length == the parent sequins
    local t = {}
    for i=1,n do
        t[i] = self()
    end
    return S.new(t)
end

local function turtle(t, fn)
    -- apply fn to all nested sequins
    fn = fn or S.next -- default to S.next
    if S.is_sequins(t) then return fn(t) end -- unwrap
    return t -- literal value
end

------------------------------
--- transformers

function S:func(fn, ...)
    self.fun = {fn, {...}} -- capture function & any args (sequins)
    return self
end

local function do_transform(s, v)
    if s.fun[1] then -- implicitly handles a cleared function
        if #s.fun[2] > 0 then
            return s.fun[1](v, table.unpack(s.fun[2]))
        else return s.fun[1](v) end
    else return v end
end

-- support arithmetic via math operators
S._fns = {
    add = function(n,b) return n+turtle(b) end,
    sub = function(n,b) return n-turtle(b) end,
    mul = function(n,b) return n*turtle(b) end,
    div = function(n,b) return n/turtle(b) end,
    mod = function(n,b) return n%turtle(b) end,
}

-- assume 'a' is self of a sequins
S.__add = function(a,b) return S.func(a, S._fns.add, b) end
S.__sub = function(a,b) return S.func(a, S._fns.sub, b) end
S.__mul = function(a,b) return S.func(a, S._fns.mul, b) end
S.__div = function(a,b) return S.func(a, S._fns.div, b) end
S.__mod = function(a,b) return S.func(a, S._fns.mod, b) end

------------------------------
--- control flow execution

local function do_step(s)
    -- if .qix is set, it will be used, rather than incrementing by s.n
    local newix = wrap_index(s, s.qix or s.ix + turtle(s.n))
    -- pull data from new index (unwrap if it's a sequins)
    local retval, exec = turtle(s.data[newix])
    -- handle messaging from child sequins
    if exec ~= 'again' then s.ix = newix; s.qix = nil end
    -- FIXME add protection for list of dead sequins. for now we just recur, hoping for a live sequin in nest
    if retval == 'skip' or retval == 'dead' then return S.next(s) end
    return retval, exec
end

S.flows = {
    every = function(f,n) return (f.ix % n) ~= 0 end,
    times = function(f,n) return f.ix > n end,
    count = function(f,n)
        if f.ix < n then return 'again'
        else f.ix = 0 end
    end
}

local function do_flow(s, k)
    local f = s.flw[k] -- check if flow-mod exists
    if f then
        f.ix = f.ix + 1
        return S.flows[k](f, turtle(f.n))
    end
end

function S.next(s)
    if do_flow(s, 'every') then return 'skip' end
    if do_flow(s, 'times') then return 'dead' end
    local again = do_flow(s, 'count')
    if again then
        local e = s.flw.every
        if e then e.ix = e.ix - 1 end -- undo every advance
    end
    return do_transform(s,do_step(s)), again
end

function S:step(n) self.n = n; return self end

function S.flow(s, k, n) s.flw[k] = {n=n, ix=0}; return s end
function S:every(n) return self:flow('every',n) end
function S:count(n) return self:flow('count',n) end
function S:times(n) return self:flow('times',n) end
function S:all() return self:flow('count',#self) end

function S:select(ix)
    rawset(self, 'qix', ix) -- qix may be nil, hence rawset
    return self
end

function S:reset()
    self:select(1)
    for _,v in ipairs(self.data) do turtle(v, S.reset) end
    for _,v in pairs(self.flw) do
        v.ix = 0
        turtle(v.n, S.reset)
    end
    if self.fun[1] and #self.fun[2] > 0 then
        for _,v in pairs(self.fun[2]) do
            turtle(v, S.reset)
        end
    end
end


------------------------------
--- metamethods

-- calling the sequins library will create a new sequins object (S:new)
-- calling a sequins object will produce a new value (S:next)
S.__call = function(self, ...)
    return (self == S) and S.new(...) or S.next(self)
end

S.metaix = { settable = S.setdata
           , step     = S.step
           , flow     = S.flow
           , every    = S.every
           , times    = S.times
           , count    = S.count
           , all      = S.all
           , reset    = S.reset
           , select   = S.select
           , peek     = S.peek
           , copy     = S.copy
           , map      = S.func
           , bake     = S.bake
           }
S.__index = function(self, ix)
    if type(ix) == 'number' then return self.data[ix]
    else
        return S.metaix[ix]
    end
end

S.__newindex = function(self, ix, v)
    if type(ix) == 'number' then self.data[ix] = v
    elseif ix == 'n' then rawset(self,ix,v)
    end
end

S.__tostring = function(t)
    -- data
    local st = {}
    for i=1,t.length do
        st[i] = tostring(t.data[i])
    end
    local s = string.format('s[%i]{%s}', t.qix or t.ix, table.concat(st,','))

    -- modifiers
    for k,v in pairs(t.flw) do
        -- TODO do we need to print current counters?
        s = string.format('%s:%s[%i](%s)',s, k:sub(1,1), v.ix, tostring(v.n))
    end

    -- transformer
    if #t.fun > 0 then
        local fns = {'fn'}
        for i=1,#t.fun[2] do
            fns[i+1] = tostring(t.fun[2][i])
        end
        s = string.format('%s:map(%s)',s, table.concat(fns, ','))
    end

    return s
end

-- use memoized data length, aka #(t.data)
S.__len = function(t) return t.length end


return setmetatable(S, S)
