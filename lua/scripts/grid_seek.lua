--test grid sequencer 

engine = 'PolyPerc'

init = function()
    e.cutoff(50*2^(cutoff/12))
    e.release(0.1*2^(release/12))
    e.amp(1)
    t:start()
end

gridkey = function(x, y, state)
   if state > 0 then 
      steps[x] = y
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
  e.hz(freqs[9-steps[pos]])
  if g ~= nil then 
    gridredraw()
  end
  redraw()
end

gridredraw = function()
  g:all(1) 
  for x = 1,16 do g:led(x,steps[x],5) end 
  g:led(pos,steps[pos],15) 
  g:refresh();
end

cutoff=30
release=20

enc = function(n,delta)
    if n==1 then
        cutoff = math.min(100,math.max(0,cutoff+delta))
        e.cutoff(50*2^(cutoff/12))
    elseif n==2 then
        release = math.min(100,math.max(0,release+delta))
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
end 
