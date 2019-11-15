local m = {
  url = '',
  version = ''
}

local function check_newest()
  print("checking for update")
  m.url = util.os_capture( [[curl -s \
      https://api.github.com/repos/monome/norns/releases/latest \
      | grep "browser_download_url.*" \
      | cut -d : -f 2,3 \
      | tr -d \"]])
  print(m.url)
  m.version = m.url:match("(%d%d%d%d%d%d)")
  print("available version "..m.version)
end

local function get_update()
  m.message = "preparing..."
  menu.redraw()
  pcall(cleanup) -- shut down script
  norns.script.clear()
  print("shutting down audio...")
  os.execute("sudo systemctl stop norns-jack.service") -- disable audio
  print("clearing old updates...")
  os.execute("sudo rm -rf /home/we/update/*") -- clear old updates
  m.message = "downloading..."
  menu.redraw()
  print("starting download...")
  os.execute("wget -T 180 -q -P /home/we/update/ " .. m.url) --download
  m.message = "unpacking update..."
  menu.redraw()
  print("checksum validation...")
  m.message = "checksum validation..."
  local checksum = util.os_capture("cd /home/we/update; sha256sum -c /home/we/update/*.sha256 | grep OK")
  if checksum:match("OK") then
    print("unpacking...")
    os.execute("tar xzvf /home/we/update/*.tgz -C /home/we/update/")
    m.message = "running update..."
    menu.redraw()
    print("running update...")
    os.execute("/home/we/update/"..m.version.."/update.sh")
    m.message = "complete. any key to shut down."
    menu.redraw()
    print("update complete.")
  else
    print("update failed.")
    m.message = "update failed."
    menu.redraw()
  end
end

m.key = function(n,z)
  if m.stage=="init" and z==1 then
    menu.set_page("SYSTEM")
    menu.redraw()
  elseif m.stage=="confirm" then
    if n==2 and z==1 then
      menu.set_page("SYSTEM")
      menu.redraw()
    elseif n==3 and z==1 then
      m.stage="update"
      get_update()
      m.stage="done"
    end
  elseif m.stage=="done" and z==1 then
    print("shutting down.")
    m.message = "shutting down."
    menu.redraw()
    os.execute("sleep 0.5; sudo shutdown now")
  end
end


m.enc = function(n,delta) end

m.redraw = function()
  screen.clear()
  screen.level(15)
  screen.move(64,40)
  if m.stage == "init" then
    screen.text_center(m.message)
  elseif m.stage == "confirm" then
    screen.text_center("update found: "..m.version)
    screen.move(64,50)
    screen.text_center("install?")
  elseif m.stage == "update" then
    screen.text_center(m.message)
  end
  screen.update()
end

m.init = function()
  m.stage = "init"

  local ping = util.os_capture("ping -c 1 github.com | grep failure")
  if ping == '' then check_newest() end

  if not ping == ''  then
    m.message = "need internet."
  elseif tonumber(norns.version.update) >= tonumber(m.version) then
    m.message = "up to date."
  elseif norns.disk < 400 then
    m.message = "disk full. need 400M."
  else
    m.stage = "confirm"
  end
end

m.deinit = function() end

return m

