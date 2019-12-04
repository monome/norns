local fileselect = require 'fileselect'

local m = {
  pos = 0,
  oldpos = 0,
  group = false,
  alt = false
}

local page

-- called from menu on script reset
m.reset = function()
  page = nil
  m.pos = 0
  m.group = false
  m.read_pmap()
end

function build_page()
  page = {}
  local i = 1
  repeat
    table.insert(page, i)
    if params:t(i) == params.tGROUP then
      i = i + params:get(i) + 1
    else i = i+1 end
  until i > params.count
end

function build_sub(sub)
  page = {}
  for i = 1,params:get(sub) do
    table.insert(page, i + sub)
  end
end


m.key = function(n,z)
  if n==1 and z==1 then
    m.alt = true
  elseif n==1 and z==0 then
    m.alt = false
  elseif n==2 and z==1 then
    if m.group==true then
      m.group = false
      build_page()
      m.pos = m.oldpos
    end
  elseif n==3 and z==1 then
    local i = page[m.pos+1]
    local t = params:t(i)
    m.fine = true
    if params.count > 0 then
      if t == params.tGROUP then
        build_sub(i)
        m.group = true
        m.oldpos = m.pos
        m.pos = 0
      elseif t == params.tSEPARATOR then
        local n = i
        repeat
          n = n+1
          if n > #page then n = 1 end
        until params:t(page[n]) == params.tSEPARATOR
        m.pos = n-1
      elseif t == params.tFILE then
        fileselect.enter(_path.dust, m.newfile)
      elseif t == params.tTRIGGER then
        params:set(i)
        m.triggered[i] = 2
      end
    end
  elseif n==3 and z==0 then
    m.fine = false
  end
  _menu.redraw()
end

m.newfile = function(file)
  if file ~= "cancel" then
    params:set(m.pos+1,file)
    _menu.redraw()
  end
end

m.enc = function(n,d)
  -- normal scroll
  if n==2 and m.alt==false then
    local prev = m.pos
    m.pos = util.clamp(m.pos + d, 0, #page - 1)
    if m.pos ~= prev then _menu.redraw() end
    -- jump section
  elseif n==2 and m.alt==true then
    d = d>0 and 1 or -1
    local i = m.pos+1
    repeat
      i = i+d
      if i > #page then i = 1 end
      if i < 1 then i = #page end
    until params:t(page[i]) == params.tSEPARATOR
    m.pos = i-1
  -- adjust value
  elseif n==3 and params.count > 0 then
    local dx = m.fine and (d/20) or d
    params:delta(page[m.pos+1],dx)
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()
  _menu.draw_panel()

  if(#page < 1) then
    screen.move(0,10)
    screen.level(4)
    screen.text("no parameters")
  else
    for i=1,6 do
      if (i > 2 - m.pos) and (i < #page - m.pos + 3) then
        if i==3 then screen.level(15) else screen.level(4) end
        local p = page[i+m.pos-2]
          if params:t(p) == params.tSEPARATOR then
            screen.move(0,10*i+2.5)
            screen.line_rel(127,0)
            screen.stroke()
            screen.move(63,10*i)
            screen.text_center(params:get_name(p))
          elseif params:t(p) == params.tGROUP then
            screen.move(0,10*i)
            screen.text(params:get_name(p) .. " >")
          else
            screen.move(0,10*i)
            screen.text(params:get_name(p))
            screen.move(127,10*i)
            if params:t(p) ==  params.tTRIGGER then
              if m.triggered[p] and m.triggered[p] > 0 then
                screen.rect(124, 10 * i - 4, 3, 3)
                screen.fill()
              end
            else
              screen.text_right(params:string(p))
            end
          end
      end
    end
  end
  screen.update()
end

m.init = function()
  if page == nil then build_page() end
  m.alt = false
  m.fine = false
  m.triggered = {}
  _menu.timer.event = function()
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
  _menu.timer:stop()
end

norns.menu_midi_event = function(data)
  if data[1] == 176 then -- cc
    if m.midilearn then
      if params:t(m.pos+1) == params.tCONTROL or params:t(m.pos+1) == params.tTAPER then
        --m.map[m.pos+1] = data[2]
        _menu.redraw()
      end
      m.midilearn = false
    else
      --local p = tab.key(m.map,data[2])
      --if p then
        --params:set_raw(p,data[3]/127)
      --end
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
        --m.map[tonumber(unquote(name),10)] = tonumber(value)
      end
    end
  else
    --print("m.read: "..filename.." not read.")
  end
end

return m
