local Mods = {}

Mods.search_pattern = "*/lib/mod.lua"

function Mods.scan(root, pattern)
  local r = root or paths.code
  local p = pattern or Mods.search_pattern

  local matches, error = norns.system_glob(r .. p)
  if not matches then return nil end

  local mods = {}
  local name_pattern = "^" .. r .. "(%w+)/"
  for i, path in ipairs(matches) do
    -- strip off path root and trailing .lua
    local relative = string.gsub(string.gsub(path, r, ""), ".lua", "")
    local _, _, mod_name = string.find(relative, "^(%w+)/")
    mods[mod_name] = {relative, path}
  end

  return mods
end

local function to_set(things)
  local s = {}
  for _, v in ipairs(things) do
    s[v] = true
  end
  return s
end

function Mods.load(scan, enabled)
  local _load = function(name, package_path)
    print('loading mod: ' .. name .. ' (' .. package_path ..')')
    require(package_path)
  end

  if enabled ~= nil then
    enabled = to_set(enabled)
  end

  for name, details in pairs(scan) do
    if enabled then
      if enabled[name] then
        _load(name, details[1])
      end
    else
      _load(name, details[1])
    end
  end
end

return Mods