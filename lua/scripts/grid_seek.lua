--test grid sequencer 

engine = 'PolyPerc'

init = function()
    e.cutoff(50*2^(cutoff/12))
    e.release(0.1*2^(release/12))
    e.amp(0.5)
    report_polls()
    t:start()
end

gridkey = function(x, y, state)
   if state > 0 then 
      if steps[x] == y then
          steps[x] = 0
      else
          steps[x] = y
      end
   end
   g:refresh()
end

t = metro[1]
t.time = 0.1

pos = 1

steps = {}
notes = {0,2,3,5,7,9,10,12}
freqs = {}

for i=1,8 do freqs[i] = 100*2^(notes[i]/12) end
for i=1,16 do steps[i] = math.floor(math.random()*8+1) end

t.callback = function(stage)
  pos = pos + 1
  if pos == 17 then pos = 1 end
  if steps[pos] > 0 then e.hz(freqs[9-steps[pos]]) end
  if g ~= nil then 
    gridredraw()
  end
  redraw()
end

gridredraw = function()
  g:all(1) 
  for x = 1,16 do
      if steps[x] > 0 then g:led(x,steps[x],5) end 
  end
  if steps[pos] > 0 then
      g:led(pos,steps[pos],15) 
  else
      g:led(pos,1,3)
  end
  g:refresh();
end

cutoff=30
release=20

enc = function(n,delta)
    if n==1 then
        cutoff = math.min(100,math.max(0,cutoff+delta))
        e.cutoff(50*2^(cutoff/12))
    elseif n==2 then
        release = math.min(60,math.max(0,release+delta))
        e.release(0.1*2^(release/12))
    end

    redraw() 
end

redraw = function()
    s.clear()
    s.level(15)
    s.move(0,10)
    s.text("cutoff > "..string.format('%.1f',(50*2^(cutoff/12))))
    s.move(0,20)
    s.text("release > "..string.format('%.3f',0.1*2^(release/12)))
    s.move(0,60)
    s.text("step > "..pos)
    s.move(0,40)
    s.aa(1)
    s.line(vu,40)
    s.stroke()
end 

p = nil
vu = 0

local function calcMeter(amp, n, floor)
   n = n or 64
   floor = floor or -72
   local db = 20.0 * math.log10(amp)
   local norm = 1.0 - (db / floor)
   local x = norm * n
   vu = x
   redraw()
end

local ampCallback = function(amp) calcMeter(amp, 64, -72) end

poll.report = function(polls)
   p = polls['amp_out_l']
   if p then
      p.callback = ampCallback
      p.time = 0.03;
      p:start()
   else
      print("couldn't get requested poll, dang")
   end 
end 


cleanup = function()
   if p then p:stop() end
end 
