local m = {
  confirmed = false
}

m.key = function(n,z)
  if n==2 and z==1 then
    _menu.set_page("SYSTEM")
  elseif n==3 and z==1 then
    m.confirmed = true
    _menu.redraw()
    os.execute("rm ~/dust/data/system.pset")
    os.execute("rm ~/dust/data/system.state")
    _norns.reset()
  end
end


m.enc = function(n,delta) end

m.redraw = function()
  screen.clear()
  screen.level(m.confirmed==false and 10 or 2)
  screen.move(64,40)
  screen.text_center(m.confirmed==false and "reset?" or "reset")
  screen.update()
end

m.init = function() end
m.deinit = function() end

return m
