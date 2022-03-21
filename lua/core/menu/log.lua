local m = {}

m.init = function()
  go = false
end

m.key = function(n,z)
  if (n==2 or n==3) and z==1 then
    print("export logs!")
    os.execute("echo '//////// current boot\n\n' > ~/dust/data/system.log")
    os.execute("journalctl --output=with-unit --boot=-0 >> ~/dust/data/system.log")
    os.execute("echo '\n\n//////// previous boot\n\n' >> ~/dust/data/system.log")
    os.execute("journalctl --output=with-unit --boot=-1 >> ~/dust/data/system.log")
    _menu.set_page("SYSTEM")
  end
end

m.enc = norns.none

m.redraw = function()
  screen.clear()
  screen.move(64,40)
  screen.level(15)
  screen.text_center("logs exported!")
  screen.update()
end

m.deinit = norns.none

return m
