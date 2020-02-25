local fileselect = require 'fileselect'
local textentry = require 'textentry'

local mSELECT = 0
local mEDIT = 1
local mPSET = 2
local mMAP = 3
local mMAPEDIT = 4

local m = {
  pos = 0,
  oldpos = 0,
  group = false,
  alt = false,
  mode = mSELECT,
  mode_prev = mSELECT,
  mode_pos = 1,
  map = false,
  mpos = 1,
  dev = 1,
  ch = 1,
  cc = 100,
  pm,
  ps_pos = 0,
  ps_n = 0,
  ps_action = 1,
  ps_last = 0
}

local page
local mode_item = { "EDIT >", "PSET >", "MAP >"}
local pset = {}

-- called from menu on script reset
m.reset = function()
  page = nil
  m.pos = 0
  m.group = false
  m.ps_pos = 0
  m.ps_n = 0
  m.ps_action = 1
  m.ps_last = 0
end

local function build_page()
  page = {}
  local i = 1
  repeat
    if params:visible(i) == false then table.insert(page, i) end
    if params:t(i) == params.tGROUP then
      i = i + params:get(i) + 1
    else i = i + 1 end
  until i > params.count
end

local function build_sub(sub)
  page = {}
  for i = 1,params:get(sub) do
    table.insert(page, i + sub)
  end
end

local function pset_list(results)
  pset = {}
  m.ps_n = 0

  local t = {}
  for filename in results:gmatch("[^\r\n]+") do
    table.insert(t,filename)
  end

  for _,file in pairs(t) do
    local n = string.gsub(file,'.pset','')
    n = tonumber(string.sub(n,-2,-1))
    if not n then n=1 end
    --print(file,n)
    local name = norns.state.shortname
    local f = io.open(file,"r")
    io.input(file)
    local line = io.read("*line")
    if util.string_starts(line, "-- ") then
      name = string.sub(line,4,-1)
    end
    io.close(f)
    pset[n] = {file=file,name=name}
    m.ps_n = math.max(n, m.ps_n)
  end

  m.ps_n = m.ps_n + 1
  _menu.redraw()
end
  
local function init_pset()
  print("scanning psets...")
  norns.system_cmd('ls -1 '..norns.state.data..norns.state.shortname..'*.pset | sort', pset_list)
end

local function write_pset(name)
  if name ~= "cancel" then
    params:write(m.ps_pos+1,name)
    m.ps_last = m.ps_pos+1
    init_pset()
    norns.pmap.write() -- write parameter map too
  end
end


m.key = function(n,z)
  if n==1 and z==1 then
    m.alt = true
  elseif n==1 and z==0 then
    m.alt = false
  -- MODE MENU
  elseif m.mode == mSELECT then
    if n==3 and z==1 then
      if m.mode_pos == 1 then
        m.mode = mEDIT
      elseif m.mode_pos == 2 then
        init_pset()
        m.mode = mPSET
      elseif m.mode_pos == 3 then
        m.mode = mMAP
      end
    end
  -- EDIT + MAP
  elseif m.mode == mEDIT or m.mode == mMAP then
    if n==2 and z==1 then
      if m.alt then
        init_pset()
        m.mode_prev = m.mode
        m.mode = mPSET
      elseif m.group==true then
        m.group = false
        build_page()
        m.pos = m.oldpos
      else
        m.mode = mSELECT
      end
    elseif n==3 and z==1 and m.alt then
      m.mode = (m.mode==mEDIT) and mMAP or mEDIT
    elseif n==3 and z==1 then
      local i = page[m.pos+1]
      local t = params:t(i)
      if t == params.tGROUP then
        build_sub(i)
        m.group = true
        m.groupname = params:string(i)
        m.oldpos = m.pos
        m.pos = 0
      elseif t == params.tSEPARATOR then
        local n = m.pos+1
        repeat
          n = n+1
          if n > #page then n = 1 end
        until params:t(page[n]) == params.tSEPARATOR
        m.pos = n-1
      elseif t == params.tFILE then
        if m.mode == mEDIT then fileselect.enter(_path.dust, m.newfile) end
      elseif t == params.tTEXT then
        if m.mode == mEDIT then
          textentry.enter(m.newtext, params:get(i), "PARAM: "..params:get_name(i))
        end
      elseif t == params.tTRIGGER then
        if m.mode == mEDIT then
          params:set(i)
          m.triggered[i] = 2
        end
      elseif m.mode == mMAP then
        local n = params:get_id(i)
        local pm = norns.pmap.data[n]
        if pm == nil then
          norns.pmap.new(n)
          pm = norns.pmap.data[n]
          local t = params:t(i)
          if t == params.tNUMBER or t == params.tOPTION then
            local r = params:get_range(i)
            pm.out_lo = r[1]
            pm.out_hi = r[2]
          end
        end
        m.dev = pm.dev
        m.ch = pm.ch
        m.cc = pm.cc
        m.mpos = 1
        m.mode = mMAPEDIT
        m.pm = pm
      end
      m.fine = true
    elseif n==3 and z==0 then
      m.fine = false
    end
    -- MAPEDIT
  elseif m.mode == mMAPEDIT then
    local p = page[m.pos+1]
    local name = params:get_id(p)
    if n==2 and z==1 then
      m.mode = mMAP
      norns.pmap.assign(name,m.dev,m.ch,m.cc)
    elseif n==3 and z==1 then
      if m.mpos == 1 then
        m.midilearn = not m.midilearn
      elseif m.mpos ==2 then
        norns.pmap.remove(name)
        m.mode = mMAP
      end
    end
    -- PSET
  elseif m.mode == mPSET then
    if n==2 and z==1 then
      m.mode = m.alt and m.mode_prev or mSELECT
    elseif n==3 and z==1 then
      -- save
      if m.ps_action == 1 then
        textentry.enter(write_pset, params.name, "PSET NAME: "..m.ps_pos+1)
        -- load
      elseif m.ps_action == 2 then
        if pset[m.ps_pos+1] then
          params:read(m.ps_pos+1)
          m.ps_last = m.ps_pos+1
        end
        -- delete
      elseif m.ps_action == 3 then
        if pset[m.ps_pos+1] then
          os.execute("rm "..pset[m.ps_pos+1].file)
          init_pset()
        end
      end
    end
  end
  _menu.redraw()
end

m.newfile = function(file)
  if file ~= "cancel" then
    params:set(page[m.pos+1],file)
    _menu.redraw()
  end
end

m.newtext = function(txt)
  print("SET TEXT: "..txt)
  if txt ~= "cancel" then
    params:set(page[m.pos+1],txt)
    _menu.redraw()
  end
end

m.enc = function(n,d)
  -- MODE MENU
  if m.mode == mSELECT then
    local prev = m.mode_pos
    m.mode_pos = util.clamp(m.mode_pos + d, 1, 3)
    if m.mode_pos ~= prev then _menu.redraw() end
  -- EDIT
  elseif m.mode == mEDIT or m.mode == mMAP then
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
      until params:t(page[i]) == params.tSEPARATOR or i==1
      m.pos = i-1
    -- adjust value
    elseif m.mode == mEDIT and n==3 and params.count > 0 then
      local dx = m.fine and (d/20) or d
      params:delta(page[m.pos+1],dx)
      _menu.redraw()
    end
  -- MAPEDIT
  elseif m.mode == mMAPEDIT then
    if n==2 then
      m.mpos = (m.mpos+d) % 11
    elseif n==3 then
      local p = page[m.pos+1]
      local n = params:get_id(p)
      local t = params:t(p)
      local pm = norns.pmap.data[n]
      if m.mpos==0 then
        params:delta(page[m.pos+1],d)
      elseif m.mpos==3 then
        m.cc = util.clamp(m.cc+d,0,127)
      elseif m.mpos==4 then
        m.ch = util.clamp(m.ch+d,1,16)
      elseif m.mpos==5 then
        m.dev = util.clamp(m.dev+d,1,16)
      elseif m.mpos==6 then
        pm.in_lo = util.clamp(pm.in_lo+d, 0, pm.in_hi)
        pm.value = util.clamp(pm.value, pm.in_lo, pm.in_hi)
      elseif m.mpos==7 then
        pm.in_hi = util.clamp(pm.in_hi+d, pm.in_lo, 127)
        pm.value = util.clamp(pm.value, pm.in_lo, pm.in_hi)
      elseif m.mpos==8 then
        pm.out_lo = pm.out_lo + d
      elseif m.mpos==9 then
        pm.out_hi = pm.out_hi + d
      elseif m.mpos==10 then
        if d>0 then pm.accum = true else pm.accum = false end
      end
    end
    _menu.redraw()
  -- PSET
  elseif m.mode == mPSET then
    if n==2 then
      m.ps_action = util.clamp(m.ps_action + d, 1, 3)
    elseif n==3 then
      m.ps_pos = util.clamp(m.ps_pos + d, 0, m.ps_n-1)
    end
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()
  _menu.draw_panel()

  -- SELECT
  if m.mode == mSELECT then
    screen.level(4)
    screen.move(0,10)
    screen.text("PARAMETERS")
    for i=1,3 do
      if i==m.mode_pos then screen.level(15) else screen.level(4) end
      screen.move(0,10*i+20)
      screen.text(mode_item[i])
    end
  -- EDIT
  elseif m.mode == mEDIT then
    if m.pos == 0 then
      local n = "PARAMETERS"
      if m.group then n = n .. " / " .. m.groupname end
      screen.level(4)
      screen.move(0,10)
      screen.text(n)
    end
    for i=1,6 do
      if (i > 2 - m.pos) and (i < #page - m.pos + 3) then
        if i==3 then screen.level(15) else screen.level(4) end
        local p = page[i+m.pos-2]
        local t = params:t(p)
        if t == params.tSEPARATOR then
          screen.move(0,10*i+2.5)
          screen.line_rel(127,0)
          screen.stroke()
          screen.move(63,10*i)
          screen.text_center(params:get_name(p))
        elseif t == params.tGROUP then
          screen.move(0,10*i)
          screen.text(params:get_name(p) .. " >")
        else
          screen.move(0,10*i)
          screen.text(params:get_name(p))
          screen.move(127,10*i)
          if t ==  params.tTRIGGER then
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
  -- MAP
  elseif m.mode == mMAP then
    if m.pos == 0 then
      local n = "PARAMETER MAP"
      if m.group then n = n .. " / " .. m.groupname end
      screen.level(4)
      screen.move(0,10)
      screen.text(n)
    end
    for i=1,6 do
      if (i > 2 - m.pos) and (i < #page - m.pos + 3) then
        if i==3 then screen.level(15) else screen.level(4) end
        local p = page[i+m.pos-2]
        local t = params:t(p)
        local n = params:get_name(p)
        local id = params:get_id(p)
        if t == params.tSEPARATOR then
          screen.move(0,10*i+2.5)
          screen.line_rel(127,0)
          screen.stroke()
          screen.move(63,10*i)
          screen.text_center(n)
        elseif t == params.tGROUP then
          screen.move(0,10*i)
          screen.text(n .. " >")
        else
          screen.move(0,10*i)
          screen.text(id)
          screen.move(127,10*i)
          if t ==  params.tNUMBER or
              t == params.tCONTROL or
              t == params.tOPTION then
            local pm=norns.pmap.data[id]
            if pm then
              screen.text_right(pm.cc..":"..pm.ch..":"..pm.dev)
            else
              screen.text_right("-")
            end
          end
        end
      end
    end
  -- MAP EDIT
  elseif m.mode == mMAPEDIT then
    local p = page[m.pos+1]
    local n = params:get_id(p)
    local t = params:t(p)
    local pm = norns.pmap.data[n]

    local function hl(x) if m.mpos==x then screen.level(15) else screen.level(4) end end

    screen.move(0,10)
    hl(0)
    screen.text(n)
    screen.move(127,10)
    screen.text_right(params:string(p))
    screen.move(0,25)
    hl(1)
    if m.midilearn then screen.text("LEARNING") else screen.text("LEARN") end
    screen.move(127,25)
    hl(2)
    screen.text_right("CLEAR")

    screen.level(4)
    screen.move(0,40)
    screen.text("cc")
    screen.move(40,40)
    hl(3)
    screen.text_right(m.cc)
    screen.level(4)
    screen.move(0,50)
    screen.text("ch")
    screen.move(40,50)
    hl(4)
    screen.text_right(m.ch)
    screen.level(4)
    screen.move(0,60)
    screen.text("dev")
    screen.move(40,60)
    hl(5)
    screen.text_right(m.dev)

    screen.level(4)
    screen.move(63,40)
    screen.text("in")
    screen.move(103,40)
    hl(6)
    screen.text_right(pm.in_lo)
    screen.level(4)
    screen.move(127,40)
    hl(7)
    screen.text_right(pm.in_hi)
    screen.level(4)
    screen.move(63,50)
    screen.text("out")
    screen.move(103,50)
    hl(8)
    screen.text_right(pm.out_lo)
    screen.move(127,50)
    hl(9)
    screen.text_right(pm.out_hi)
    screen.level(4)
    screen.move(63,60)
    screen.text("accum")
    screen.move(127,60)
    hl(10)
    screen.text_right(pm.accum and "yes" or "no")
  -- PSET
  elseif m.mode == mPSET then
    screen.level(4)
    screen.move(0,10)
    screen.text("PSET")
    screen.move(0,30)
    local v = (m.ps_action == 1) and 15 or 4
    screen.level(v)
    screen.text("SAVE")
    screen.move(0,40)
    v = (m.ps_action == 2) and 15 or 4
    screen.level(v)
    screen.text("LOAD")
    screen.move(0,50)
    v = (m.ps_action == 3) and 15 or 4
    screen.level(v)
    screen.text("DELETE")
    for i=1,6 do
      local n = i+m.ps_pos-2
      if (i > 2 - m.ps_pos) and (i < m.ps_n - m.ps_pos + 3) then
        local line = "-"
        if pset[n] then line = pset[n].name end
        if(i==3) then
          screen.level(15)
        else
          screen.level(4)
        end
        screen.move(50,10*i)
        local num = (n == m.ps_last) and "*"..n or n
        screen.text_right(num)
        screen.move(56,10*i)
        screen.text(string.upper(line))
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


norns.menu_midi_event = function(data, dev)
  local ch = data[1] - 175
  local cc = data[2]
  local v = data[3]
  if ch > 0 and ch < 17 then
    if m.midilearn then
      m.midilearn = false
      m.dev = dev
      m.ch = ch
      m.cc = cc
      local p = page[m.pos+1]
      local name = params:get_id(p)
      norns.pmap.assign(name,m.dev,m.ch,m.cc)
      if _menu.mode then _menu.redraw() end
    else
      --print(cc.." : "..v)
      local r = norns.pmap.rev[dev][ch][cc] 
      if r then
        local d = norns.pmap.data[r]
        local t = params:t(r)
        if d.accum then
          v = v - 64
          d.value = util.clamp(d.value + v, d.in_lo, d.in_hi)
          v = d.value
        end
        local s = util.clamp(v, d.in_lo, d.in_hi)
        s = util.linlin(d.in_lo, d.in_hi, d.out_lo, d.out_hi, s)
        if t == params.tCONTROL or t == params.tTAPER then
          params:set_raw(r,s)
        elseif t == params.tNUMBER or t == params.tOPTION then
          s = util.round(s)
          params:set(r,s)
        end
        if _menu.mode then _menu.redraw() end
      end
    end
  end
end


return m
