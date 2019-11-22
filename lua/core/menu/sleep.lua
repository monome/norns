local m = {}

m.key = function(n,z)
  if n==2 and z==1 then
    _menu.set_page("HOME")
  elseif n==3 and z==1 then
    print("SLEEP")
    --TODO fade out screen then run the shutdown script
    m.sleep = true
    _menu.redraw()
    norns.state.clean_shutdown = true
    norns.state.save()
    pcall(cleanup)
    -- TODO
    --if m.tape.rec.sel == TAPE_REC_STOP then audio.tape_record_stop() end
    audio.level_dac(0)
    audio.headphone_gain(0)
    os.execute("sleep 0.5; sudo shutdown now")
  end
end

m.enc = norns.none

m.redraw = function()
  screen.clear()
  screen.move(48,40)
  if m.sleep then
	screen.level(1)
	screen.text("sleep.")
  else
	screen.level(15)
  screen.text("sleep?")
  end
  --TODO do an animation here! fade the volume down
  screen.update()
end

m.init = norns.none
m.deinit = norns.none

return m
