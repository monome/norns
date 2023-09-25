-- Group class
-- @module params.group

local Group = {}
Group.__index = Group

local tGROUP = 7

function Group.new(id, name, n)
  local g = setmetatable({}, Group)
  g.name = type(name) ~= "number" and name or (id or "group")
  g.id = id or g.name
  g.n = type(name) == "number" and name or (n or 1)
  g.t = tGROUP
  g.action = function() end
  return g
end

function Group:get()
  return self.n 
end

function Group:set(v) end
function Group:delta(d) end
function Group:set_default() end
function Group:bang() end

function Group:string()
  return self.name
end

return Group
