-- parameter map

local pmap = {
  data = {},
  rev = {}
}

-- build reverse lookup table: dev -> ch -> cc
for i=1,4 do 
  pmap.rev[i]={}
  for n=1,16 do
    pmap.rev[i][n]={}
  end
end

pmap.__index = pmap

function pmap.new(id)
  print("new pmap: "..id)
  local p = setmetatable({}, pmap)
  p.cc = 1
  p.ch = 1
  p.dev = 1
  p.in_lo = 0
  p.in_hi = 127
  p.out_lo = 0
  p.out_hi = 1
  p.accum = false
  pmap.data[id] = p
  pmap.assign(id,p.dev,p.ch,p.cc)
end

function pmap.remove(id)
  print("pmap remove: "..id)
  local p = pmap.data[id]
  if p then pmap.rev[p.dev][p.ch][p.cc] = nil end
  pmap.data[id] = nil
end

function pmap.assign(id, dev, ch, cc)
  local prev = pmap.rev[dev][ch][cc]
  if prev then pmap.remove(prev) end
  local p = pmap.data[id]
  pmap.rev[p.dev][p.ch][p.cc] = nil
  p.dev=dev
  p.ch=ch
  p.cc=cc
  pmap.rev[dev][ch][cc] = id
end

return pmap
