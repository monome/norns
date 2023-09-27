local m = {
  pos = 1,
  list = {"SELECT", "SYSTEM", "SLEEP"}
}

m.init = function()
  _menu.timer.time = 1
  _menu.timer.count = -1
  _menu.timer.event = function() _menu.redraw() end
  _menu.timer:start()
end

m.deinit = function()
  _menu.timer:stop()
end

m.key = function(n,z)
  if n == 2 and z == 1 then
    _menu.showstats = not _menu.showstats
    _menu.redraw()
  elseif n == 3 and z == 1 then
    if _menu.alt == false then
      _menu.set_page(m.list[m.pos])
    else
      norns.script.clear()
      _norns.free_engine()
      _menu.m["PARAMS"].reset()
      _menu.locked = true
    end
  end
end

m.enc = function(n,delta)
  if n == 2 then
    m.pos = util.clamp(m.pos + delta, 1, #m.list)
    _menu.redraw()
  end
end

local xrun_warn_count = 0
local draw_stats = function()
  screen.level(1)
  if norns.is_norns then
    screen.move(127,55)
    screen.text_right(norns.battery_current.."mA @ ".. norns.battery_percent.."%")
    --screen.move(36,10)
  end

  screen.move(0,10) screen.text("cpu")
  for i=1,4 do
    screen.move(6+(i*14),10) 
    screen.text(norns.cpu[i]..".")
  end

  screen.move(80,10)
  local cpu = math.floor(_norns.audio_get_cpu_load())
  screen.text(tostring(cpu)..'%')
  local xruns = _norns.audio_get_xrun_count()
  if xruns > 0 then
    xrun_warn_count = 4
  end
  if xrun_warn_count > 0 then 
    screen.move(98,10)
    screen.level(xrun_warn_count * 3)
    screen.text("!!!")
    screen.level(1)
    xrun_warn_count = xrun_warn_count - 1 
  end

  screen.move(127,10)
  screen.text_right(norns.temp .. "c")

  screen.move(0,20)
  screen.text("disk " .. norns.disk .. "M")
  screen.move(127,20)
  if wifi.state > 0 then
    screen.text_right(wifi.ip)
  end
  screen.move(127,45)
  screen.text_right(norns.version.update)

  screen.level(15)
  screen.move(127,35)
  screen.text_right(string.upper(norns.state.name))
end

m.redraw = function()
  screen.clear()
  _menu.draw_panel()
  for i=1,3 do
    screen.move(0,25+10*i)
    if(i==m.pos) then
      screen.level(15)
    else
      screen.level(4)
    end
    screen.text(m.list[i].." >")
  end

  if not _menu.showstats then
    local line = string.upper(norns.state.name)
    if(_menu.scripterror and _menu.errormsg ~= 'NO SCRIPT') then
      line = "error: " .. _menu.errormsg
      if util.string_starts(_menu.errormsg,"missing") then
        screen.level(8)
        screen.move(0,25)
        screen.text("try 'SYSTEM > RESTART'")
      elseif util.string_starts(_menu.errormsg,"version") then
        screen.level(8)
        screen.move(0,25)
        screen.text("try 'SYSTEM > UPDATE'")
      end
    end
    screen.level(15)
    screen.move(0,15)
    screen.text(line)
  else
    draw_stats()
  end
  
  if _menu.alt==true and m.pos==1 then
    screen.clear()
    screen.move(64,40)
    screen.level(15)
    screen.text_center("CLEAR")
  end

  screen.update()
end

return m
