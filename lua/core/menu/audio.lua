local m = {
  pos = 0
}

m.key = function(n,z)
  if n==2 and z==1 then
    _menu.set_page("SYSTEM")
  elseif n==3 and z==1 then
    if mix:t(m.pos+1) == mix.tFILE then
      fileselect.enter(_path.dust, m.newfile)
    end
  end
end

m.newfile = function(file)
  if file ~= "cancel" then
    mix:set(m.pos+1,file)
    _menu.redraw()
  end
end

m.enc = function(n,d)
  if n==2 then
    local prev = m.pos
    m.pos = util.clamp(m.pos + d, 0, mix.count - 1)
    if m.pos ~= prev then _menu.redraw() end
  elseif n==3 then
    mix:delta(m.pos+1,d)
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()
  for i=1,6 do
    if (i > 2 - m.pos) and (i < mix.count - m.pos + 3) then
      if i==3 then screen.level(15) else screen.level(4) end
      local param_index = i+m.pos-2

      if mix:t(param_index) == mix.tSEPARATOR then
        screen.move(0,10*i)
        screen.text(mix:string(param_index))
      else
        screen.move(0,10*i)
        screen.text(mix:get_name(param_index))
        screen.move(127,10*i)
        screen.text_right(mix:string(param_index))
      end
    end
  end
  screen.update()
end

m.init = function()
  _menu.timer.event = function() _menu.redraw() end
  _menu.timer.time = 1
  _menu.timer.count = -1
  _menu.timer:start()
end

m.deinit = function()
  _menu.timer:stop()
end

return m
