local tabutil = require 'tabutil'

local enabled_mods = {}
local loaded_mods = {}

local Mods = {}

--
-- loading support
--

Mods.search_pattern = "*/lib/mod.lua"

function Mods.scan(root, pattern)
  local r = root or paths.code
  local p = pattern or Mods.search_pattern

  local matches, error = norns.system_glob(r .. p)
  if not matches then return nil end

  local mods = {}
  local name_pattern = "^" .. r .. "([%w_-]+)/"
  for i, path in ipairs(matches) do
    -- strip off path root and trailing .lua
    local relative = string.gsub(string.gsub(path, r, ""), ".lua", "")
    local _, _, mod_name = string.find(relative, "^([%w_-]+)/")
    mods[mod_name] = {relative, path}
  end

  return mods
end

local function to_set(list)
  local s = {}
  if list ~= nil then
    for _, v in ipairs(list) do s[v] = true end
  end
  return s
end

local function to_list(set)
  local l = {}
  if set ~= nil then
    for k, bool in pairs(set) do
      if bool then table.insert(l, k) end
    end
  end
  return l
end

function Mods.load(scan, only_enabled)
  local _load = function(name, package_path)
    Mods.this_name = name
    print('loading mod: ' .. name .. ' (' .. package_path ..')')
    require(package_path)
    Mods.this_name = nil
    loaded_mods[name] = true
  end

  for name, details in pairs(scan) do
    if only_enabled then
      if Mods.is_enabled(name) then
        _load(name, details[1])
      end
    else
      _load(name, details[1])
    end
  end
end

function Mods.load_enabled()
  enabled_mods = to_set(tabutil.load(paths.enabled_mods)) or {}
  return enabled_mods
end

function Mods.save_enabled()
  tabutil.save(to_list(enabled_mods), paths.enabled_mods)
end

function Mods.set_enabled(name, bool, autosave)
  if enabled_mods[name] ~= bool then
    -- it changed, record and save
    enabled_mods[name] = bool
    if autosave then
      tabutil.save(to_list(enabled_mods), paths.enabled_mods)
    end
  end
end

function Mods.is_enabled(name)
  return enabled_mods[name] or false
end

function Mods.enabled_mod_names()
  return to_list(enabled_mods)
end

function Mods.is_loaded(name)
  return loaded_mods[name] or false
end

function Mods.loaded_mod_names()
  return to_list(loaded_mods)
end

--
-- menu support
--

Mods.menu = {
  redraw = function()
    _menu.redraw()
  end,

  exit = function()
    _menu.set_page("MODS")
  end,

  register = function(name, m)
    _menu.m[name] = m
  end,
}

--
-- hook support, provide function (as opposed to method) interface to pair with
-- menu support.
--

local hooks = require 'core/hook'

Mods.hook = {
  register = function(which, name, func)
    hooks[which]:register(name, func)
  end,

  deregister = function(which, name)
    hooks[which]:deregister(name)
  end,
}


return Mods