-- parameter map

local pmap = {
  data = {},
  rev = {}
}

pmap.__index = pmap

function pmap.new(id)
  local p = setmetatable({}, pmap)
  p.cc = 100
  p.ch = 1
  p.dev = 1
  p.in_lo = 0
  p.in_hi = 127
  p.out_lo = 0
  p.out_hi = 1
  p.accum = false
  p.value = 0
  pmap.data[id] = p
end

function pmap.remove(id)
  local p = pmap.data[id]
  if p then pmap.rev[p.dev][p.ch][p.cc] = nil end
  pmap.data[id] = nil
end

function pmap.assign(id, dev, ch, cc)
  local prev = pmap.rev[dev][ch][cc]
  if prev and prev ~= id then 
    pmap.remove(prev) end
  local p = pmap.data[id]
  pmap.rev[p.dev][p.ch][p.cc] = nil
  p.dev=dev
  p.ch=ch
  p.cc=cc
  pmap.rev[dev][ch][cc] = id
end

function pmap.refresh()
  for k,v in pairs(pmap.data) do
    pmap.rev[v.dev][v.ch][v.cc] = k
  end
end

function pmap.clear()
  pmap.data = {}
  pmap.rev = {}
  -- build reverse lookup table: dev -> ch -> cc
  for i=1,4 do
    pmap.rev[i]={}
    for n=1,16 do
      pmap.rev[i][n]={}
    end
  end
end

function pmap.write()
  local function quote(s)
    return '"'..s:gsub('"', '\\"')..'"'
  end
  local filename = norns.state.data..norns.state.shortname..".pmap"
  print(">> saving PMAP "..filename)
  local fd = io.open(filename, "w+")
  io.output(fd)
  local line = ""
  for k,v in pairs(pmap.data) do
    line = string.format('%s:"{', quote(tostring(k)))
    for x,y in pairs(v) do
      line = line..x.."="..tostring(y)..", "
    end
    line = line:sub(1,-3)..'}"\n'
    --print(line)
    io.write(line)
    line=""
  end
  io.close(fd)
end

function pmap.read()
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
      if name and value and tonumber(value)==nil then
        --print(unquote(name) .. " : " .. unquote(value))
        local x = load("return "..unquote(value))
        pmap.data[unquote(name)] = x()
      end
    end
    pmap.refresh()
  else
    print("m.read: "..filename.." not read.")
  end
end


pmap.clear()

return pmap
