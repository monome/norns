local m = {
  alt = false,
  install = "stable"
}

releases = {}
local VER = { "stable", "beta" }

local function checked() print("what??") end
local function get_update_2() end

local function check_newest()
  print("checking for update")
  if m.alt then print("stable and beta") end
  norns.system_cmd( [[curl -s \
      https://raw.githubusercontent.com/monome/norns/main/releases.txt \
      ]],
      checked)
end

checked = function(result)
  print(result)
  load(result)()

  if m.alt==false and tonumber(norns.version.update) >= tonumber(releases.stable.version) then
    m.message = "up to date."
  else
    m.stage = "confirm"
  end
  _menu.redraw()
end


local function get_update()
  m.message = "preparing..."
  _menu.redraw()
  pcall(cleanup) -- shut down script
  norns.script.clear()
  _menu.locked = true
  print("shutting down audio...")
  --os.execute("sudo systemctl stop norns-jack.service") -- disable audio
  os.execute("sudo systemctl stop norns-crone.service") -- disable audio
  os.execute("sudo systemctl stop norns-sclang.service") -- disable audio
  print("clearing old updates...")
  os.execute("sudo rm -rf /home/we/update/*") -- clear old updates
  m.message = "downloading..."
  _menu.redraw()
  print("starting download...")
  local cmd = "wget -T 180 -q -P /home/we/update/ " .. releases[m.install].url .. " " .. releases[m.install].sha
  print("> "..cmd)
  m.initial_disk = norns.disk
  norns.system_cmd(cmd, get_update_2) --download
  _menu.timer.time = 0.5
  _menu.timer.count = -1
  _menu.timer.event = function()
    m.blink = m.blink == false
    m.message = "downloading: " .. m.initial_disk-norns.disk
    _menu.redraw()
  end
  _menu.timer:start()
end

get_update_2 = function()
  _menu.timer:stop()
  m.message = "unpacking update..."
  _menu.redraw()
  print("checksum validation...")
  m.message = "checksum validation..."
  local checksum = util.os_capture("cd /home/we/update; sha256sum -c /home/we/update/*.sha256 | grep OK")
  if checksum:match("OK") then
    print("unpacking...")
    os.execute("tar xzvf /home/we/update/*.tgz -C /home/we/update/")
    m.message = "running update..."
    _menu.redraw()
    print("running update...")
    os.execute("/home/we/update/"..releases[m.install].version.."/update.sh")
    m.message = "done. any key to shut down."
    _menu.redraw()
    print("update complete.")
  else
    print("update failed.")
    m.message = "update failed."
    _menu.redraw()
  end
  m.stage = "done"
end


m.key = function(n,z)
  if m.stage=="init" and z==1 then
    _menu.set_page("SYSTEM")
    _menu.redraw()
  elseif m.stage=="confirm" then
    if n==2 and z==1 then
      _menu.set_page("SYSTEM")
      _menu.redraw()
    elseif n==3 and z==1 then
      m.stage="update"
      get_update()
    end
  elseif m.stage=="update" then
    if n==2 and z==1 then
      m.stage="cancel"
      _menu.redraw()
    end
  elseif m.stage=="cancel" then
    if n==2 and z==1 then
      m.stage="update"
      _menu.redraw()
    elseif n==3 and z==1 then
      _norns.reset()
    end
  elseif m.stage=="done" and z==1 then
    m.stage = "shutdown"
    print("shutting down.")
    _menu.redraw()
    os.execute("sleep 0.5; sudo shutdown now")
  end
end


m.enc = function(n,delta)
  if n==2 and delta<0 then m.install = "stable" else m.install = "beta" end
  if m.stage == "confirm" then
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()
  screen.level(15)
  screen.move(64,40)
  if m.stage == "confirm" then
    if m.alt==false then
      screen.text_center("update found: "..releases.stable.version)
      screen.move(64,50)
      screen.text_center("install?")
    else
      screen.move(0,30)
      screen.level(m.install=="stable" and 15 or 1)
      screen.text("stable-"..releases.stable.version)
      screen.move(0,40)
      screen.level(m.install=="beta" and 15 or 1)
      screen.text("beta-"..releases.beta.version)
    end
  elseif m.stage == "update" then
    screen.level(m.blink == true and 15 or 3)
    screen.text_center(m.message)
  elseif m.stage == "cancel" then
    screen.text_center("cancel?")
  elseif m.stage == "shutdown" then
    screen.text_center("shutting down.")
    if norns.is_shield then
      screen.move(64,50)
      screen.text_center("disconnect power when the")
      screen.move(64,60)
      screen.text_center("not-red light stops blinking.")
    end
  else
    screen.text_center(m.message)
  end
  screen.update()
end

m.init = function()
  m.install = "stable"
  m.alt = _menu.alt

  m.stage = "init"
  m.message = "checking for update..."
  _menu.redraw()

  local ping = util.os_capture("ping -c 1 github.com | grep failure")

  if not ping == ''  then
    m.message = "need internet."
  elseif norns.disk < 400 then
    m.message = "disk full. need 400M."
  else check_newest() end
end

m.deinit = function() end

return m

