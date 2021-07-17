--- sequins
-- nestable tables with sequencing behaviours & control flow
-- TODO i think ASL can be defined in terms of a sequins...
--
-- for the norns port, @tyleretters copy-pasta'd with no changes from:
-- https://github.com/monome/crow/blob/34ce1e455f01fdef65a0d37aa97163b4cd14a115/lua/sequins.lua

local S = {}

function S.new(t)
    -- wrap a table in a sequins with defaults
    local s = { data   = t
              , length = #t -- memoize table length for speed
              , set_ix = 1 -- force first stage to start at elem 1
              , ix     = 1 -- current val
              , n      = 1 -- can be a sequin
              }
    s.action = {up = s}
    setmetatable(s, S)
    return s
end

local function wrap_index(s, ix) return ((ix - 1) % s.length) + 1 end

-- can this be generalized to cover every/count/times etc
function S.setdata(self, t)
    self.data   = t
    self.length = #t
    self.ix = wrap_index(self, self.ix)
end

function S.is_sequins(t) return getmetatable(t) == S end

local function turtle(t, fn)
    -- apply fn to all nested sequins. default to 'next'
    if S.is_sequins(t) then
        if fn then
            return fn(t)
        else return S.next(t) end
    end
    return t
end


------------------------------
--- control flow execution

function S.next(self)
    local act = self.action
    if act.action then
        return S.do_ctrl(act)
    else return S.do_step(act) end
end

function S.select(self, ix)
    rawset(self, 'set_ix', ix)
    return self
end

function S.do_step(act)
    local s = act.up
    -- if .set_ix is set, it will be used, rather than incrementing by s.n
    local newix = wrap_index(s, s.set_ix or s.ix + turtle(s.n))
    local retval, exec = turtle(s.data[newix])
    if exec ~= 'again' then s.ix = newix; s.set_ix = nil end
    -- FIXME add protection for list of dead sequins. for now we just recur, hoping for a live sequin in nest
    if exec == 'skip' then return S.next(s) end
    return retval, exec
end


------------------------------
--- control flow manipulation

function S.do_ctrl(act)
    act.ix = act.ix + 1
    if not act.cond or act.cond(act) then
        retval, exec = S.next(act)
        if exec then act.ix = act.ix - 1 end -- undo increment
    else
        retval, exec = {}, 'skip'
    end
    if act.rcond then
        if act.rcond(act) then
            if exec == 'skip' then retval, exec = S.next(act)
            else exec = 'again' end
        end
    end
    return retval, exec
end

function S.reset(self)
    self.ix = self.length
    for _,v in ipairs(self.data) do turtle(v, S.reset) end
    local a = self.action
    while a.ix do
        a.ix = 0
        turtle(a.n, S.reset)
        a = a.action
    end
end

--- behaviour modifiers
function S.step(self, s) self.n = s; return self end


function S.extend(self, t)
    self.action = { up     = self -- containing sequins
                  , action = self.action -- wrap nested actions
                  , ix     = 0
                  }
    for k,v in pairs(t) do self.action[k] = v end
    return self
end

function S._every(self)
    return (self.ix % turtle(self.n)) == 0
end

function S._times(self)
    return self.ix <= turtle(self.n)
end

function S._count(self)
    if self.ix < turtle(self.n) then return true
    else self.ix = 0 end -- reset
end

function S.cond(self, p) return S.extend(self, {cond = p}) end
function S.condr(self, p) return S.extend(self, {cond = p, rcond = p}) end
function S.every(self, n) return S.extend(self, {cond = S._every, n = n}) end
function S.times(self, n) return S.extend(self, {cond = S._times, n = n}) end
function S.count(self, n) return S.extend(self, {rcond = S._count, n = n}) end

--- helpers in terms of core
function S.all(self) return self:count(self.length) end
function S.once(self) return self:times(1) end


--- metamethods

S.__call = function(self, ...)
    return (self == S) and S.new(...) or S.next(self)
end

S.metaix = { settable = S.setdata
           , step     = S.step
           , cond     = S.cond
           , condr    = S.condr
           , every    = S.every
           , times    = S.times
           , count    = S.count
           , all      = S.all
           , once     = S.once
           , reset    = S.reset
           , select   = S.select
           }
S.__index = function(self, ix)
    -- runtime calls to step() and select() should return values, not functions
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


setmetatable(S, S)

return S
