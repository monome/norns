local map = {}
local low = 31
local high = 112
local gamma = 1.0
local lock = 0

map[0] = {
  position = {x=0, y=5},
  name = "LOW",
  get = function(_) return low end,
  set = function(d) low = util.clamp(0, low + d, 31) end,
  is_default = function(_) return low == 31 end,
  tip = {
    "The pre-charge voltage level.",
    "0-31.",
  },
}
map[1] = {
  position = {x=22, y=5},
  name = "HIGH",
  get = function(_) return high end,
  set = function(d) high = util.clamp(1, high + d, 180) end,
  is_default = function(_) return high == 112 end,
  tip = {
   "The max voltage level, values",
   "above 112 may shorten the",
   "lifespan of the screen. 1-180",
  },
}
map[2] = {
  position = {x=48, y=5},
  name = "GAMMA",
  get = function(_) return string.format("%.2f", gamma) end,
  set = function(d) gamma = util.clamp(0.0, gamma + (d * 0.01),  30.0) end,
  is_default = function(_) return gamma > 0.999 and gamma < 1.001 end,
  tip = {
    "The gamma curve. Try 2.2 or",
    "2.4 for gentle gradients.",
  },
}
map[3] = {
  position = {x=106, y=5},
  name = "LOCK?",
  get = function(_) return lock == 1 and "YES" or "NO" end,
  set = function(d) lock = util.clamp(0, lock + d,  1) end,
  is_default = function(_) return false end,
  tip = {
    "Turning this on will prevent",
    "scripts from changing gamma.",
  }
}

local load_settings = function()
  local executable_lua, err = loadfile(_path.display_settings)
  if err then return err end

  local loaded_settings = executable_lua() or {}
  low   = loaded_settings.low   or  31
  high  = loaded_settings.high  or 112
  gamma = loaded_settings.gamma or   1.0
  lock  = loaded_settings.lock  or   0
end

local save_settings = function()
  local file, err = io.open(_path.display_settings, "w")
  if err then return err end
  local s = ""
  s = s.."low="..low..","
  s = s.."high="..high..","
  s = s.."gamma="..gamma..","
  s = s.."lock="..lock..","
  file:write("return {"..s.."}")
  file:close()
end

local m = {
  list = {
    map[0].name,
    map[1].name,
    map[2].name,
    map[3].name,
  },
  pos = 0,
  len = tab.count(map),
}

m.key = function(n,z)
  if n==2 and z==1 then
    _menu.set_page("SYSTEM")
  end
end

m.enc = function(n,delta)
  if n==2 then
    m.pos = util.clamp(0, m.pos + delta, m.len - 1)
    _menu.redraw()
  elseif n==3 then
    map[m.pos].set(delta)
    _norns.screen_precharge(low)
    _norns.screen_gamma(gamma)
    save_settings()
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()

  for i=0,15 do
    screen.level(i)
    screen.rect(i*8, 64, 8, -13)
    screen.fill()
    screen.level(i <= 7 and 15 or 0)
    screen.move(i*8 + 4, 63)
    screen.text_center(i)
  end

  for i=0, (m.len - 1) do
    local setting = map[i]

    if i == m.pos then
      screen.level(8)
      screen.move(64, 26)
      for _, line in pairs(setting.tip) do
        screen.text_center(line)
	screen.move_rel(-(screen.text_extents(line)//2), 8)
      end
    end

    screen.level(i == m.pos and 15 or 4)
    screen.move(setting.position.x, setting.position.y)
    screen.text(setting.name)
    screen.text(setting.is_default() and "*" or "")
    screen.move(setting.position.x, setting.position.y + 8)
    screen.text(setting.get())
  end

  screen.update()
end

m.init = function()
  load_settings()
  -- screen enter notification
  m.selected = 1
  _menu.timer.time = 3
  _menu.timer.count = -1
  _menu.timer.event = function()
    _menu.redraw()
  end
  _menu.timer:start()
end

m.deinit = function()
  _menu.timer:stop()
end

return m
