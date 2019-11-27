local fileselect = require 'fileselect'

local m = {
  pos = 0,
  n = 0,
  loadable = true,
  altpos = 1,
  map = {}
}

m.init_map = function()
  for i = 1,params.count do m.map[i] = -1 end
end


m.key = function(n,z)
  if _menu.alt then
    if n==3 and z==1 then
      if m.altpos == 1 and m.loadable==true then
        params:read(m.n)
        m.action = 15
        m.action_text = "loaded"
      elseif m.altpos == 2 then
        params:write(m.n)
        m.action = 15
        m.action_text = "saved"
        m.loadable = true
        -- save mapping
        m.write_pmap()
      end
      _menu.redraw()
    end
  elseif n==2 and z==1 then
    --NOT USED
    --_menu.set_page(pHOME)
  elseif n==3 and z==1 then
    if not m.midimap then
      m.fine = true
      if params.count > 0 then
        if params:t(m.pos+1) == params.tFILE then
          fileselect.enter(_path.dust, m.newfile)
        elseif params:t(m.pos+1) == params.tTRIGGER then
          params:set(m.pos+1)
          m.triggered[m.pos+1] = 2
        end
      end
    else
      m.midilearn = not m.midilearn
    end
  elseif n==3 and z==0 then
    m.fine = false
  end
end

m.newfile = function(file)
  if file ~= "cancel" then
    params:set(m.pos+1,file)
    _menu.redraw()
  end
end

m.enc = function(n,d)
  if _menu.alt then
    if n == 2 then
      m.altpos = util.clamp(m.altpos+d, 1, 3)
      _menu.redraw()
    elseif n==3 then
      if m.altpos < 3 then
        m.n = util.clamp(m.n + d,0,100)
        local path
        local f
        if m.n == 0 then
          path = norns.state.data .. norns.state.shortname..".pset"
          f=io.open(path,"r")
        else
          path = norns.state.data .. norns.state.shortname.."-"..string.format("%02d",m.n)..".pset"
          f=io.open(path ,"r")
        end
        --print("pset: "..path)
        if f~=nil then
          m.loadable = true
          io.close(f)
        else
          m.loadable = false
        end
        _menu.redraw()
      else
        m.midimap = d > 0
        _menu.redraw()
      end
    end
  elseif n==2 then
    local prev = m.pos
    m.pos = util.clamp(m.pos + d, 0, params.count - 1)
    if m.pos ~= prev then _menu.redraw() end
  elseif n==3 and params.count > 0 then
    if not m.midimap then
      local dx = m.fine and (d/20) or d
      params:delta(m.pos+1,dx)
      _menu.redraw()
    else
      m.map[m.pos+1] = util.clamp(m.map[m.pos+1]+d,-1,127)
      _menu.redraw()
    end
  end
end

m.redraw = function()
  screen.clear()

  _menu.draw_panel()

  if(params.count > 0) then
    if not _menu.alt then
      for i=1,6 do
        if (i > 2 - m.pos) and (i < params.count - m.pos + 3) then
          if i==3 then screen.level(15) else screen.level(4) end
          local param_index = i+m.pos-2

          if params:t(param_index) == params.tSEPARATOR then
            screen.move(0,10*i)
            screen.text(params:string(param_index))
          else
            screen.move(0,10*i)
            screen.text(params:get_name(param_index))
            if m.midimap then
              if params:t(param_index) == params.tCONTROL or params:t(param_index) == params.tTAPER then
                screen.move(127,10*i)
                if m.map[param_index] >= 0 then
                  screen.text_right(m.map[param_index])
                else
                  screen.text_right("-")
                end
              end
            else
              screen.move(127,10*i)
              if params:t(param_index) ==  params.tTRIGGER then
                if m.triggered[param_index] and m.triggered[param_index] > 0 then
                  screen.rect(124, 10 * i - 4, 3, 3)
                  screen.fill()
                end
              else
                screen.text_right(params:string(param_index))
              end
            end
          end
        end
      end
      if m.midilearn then
        screen.level(15)
        screen.move(80,30)
        screen.text("(learn)")
      end
    else -- _menu.alt == true -- param save/load

      screen.level((m.altpos == 1) and 15 or 4)
      screen.move(0,30)
      screen.text("load")
      if m.altpos == 1 then
        screen.move(127,30)
        screen.level(m.loadable and 10 or 1)
        if m.n == 0 then
          screen.text_right("default")
        else
          screen.text_right(string.format("%02d",m.n))
        end
      end

      screen.level((m.altpos == 2) and 15 or 4)
      screen.move(0,40)
      screen.text("save")
      if m.altpos == 2 then
        screen.move(127,40)
        screen.level(m.loadable and 10 or 4)
        if m.n == 0 then
          screen.text_right("default")
        else
          screen.text_right(string.format("%02d",m.n))
        end
      end

      screen.level((m.altpos == 3) and 15 or 4)
      screen.move(0,50)
      screen.text("midi-cc mapping")
      screen.move(127,50)
      screen.text_right(m.midimap and "on" or "off")

      screen.move(0,10)
      screen.level(m.action)
      screen.text(m.action_text)
    end
  else
    screen.move(0,10)
    screen.level(4)
    screen.text("no parameters")
  end
  screen.update()
end

m.init = function()
  m.fine = false
  m.midimap = false
  m.midilearn = false
  m.action_text = ""
  m.action = 0
  m.triggered = {}
  _menu.timer.event = function()
    if m.action > 0 then m.action = m.action - 1 end
    for k, v in pairs(m.triggered) do
      if v > 0 then m.triggered[k] = v - 1 end
    end
    _menu.redraw()
  end
  _menu.timer.time = 0.2
  _menu.timer.count = -1
  _menu.timer:start()
end

m.deinit = function()
  m.midilearn = false
  _menu.timer:stop()
end

norns.menu_midi_event = function(data)
  if data[1] == 176 then -- cc
    if m.midilearn then
      if params:t(m.pos+1) == params.tCONTROL or params:t(m.pos+1) == params.tTAPER then
        m.map[m.pos+1] = data[2]
        _menu.redraw()
      end
      m.midilearn = false
    else
      local p = tab.key(m.map,data[2])
      if p then
        params:set_raw(p,data[3]/127)
      end
      --print(data[2] .. " " .. data[3])
    end
  end
end

function m.write_pmap()
  local function quote(s)
    return '"'..s:gsub('"', '\\"')..'"'
  end
  local filename = norns.state.data..norns.state.shortname..".pmap"
  print(">> saving PMAP "..filename)
  local fd = io.open(filename, "w+")
  io.output(fd)
  for k,v in pairs(m.map) do
    io.write(string.format("%s: %d\n", quote(tostring(k)), v))
  end
  io.close(fd)
end

function m.read_pmap()
  local function unquote(s)
    return s:gsub('^"', ''):gsub('"$', ''):gsub('\\"', '"')
  end
  local filename = norns.state.data..norns.state.shortname..".pmap"
  print(">> reading PMAP "..filename)
  local fd = io.open(filename, "r")
  if fd then
    io.close(fd)
    for line in io.lines(filename) do
      --local name, value = string.match(line, "(\".-\")%s*:%s*(.*)")
      local name, value = string.match(line, "(\".-\")%s*:%s*(.*)")

      if name and value then
        --print(unquote(name) .. " : " .. value)
        m.map[tonumber(unquote(name),10)] = tonumber(value)
      end
    end
  else
    --print("m.read: "..filename.." not read.")
  end
end

return m
