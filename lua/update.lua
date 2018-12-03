--- system update functions
local Update = {}

function Update.check()
  local found = false
  local _, _, popen = 0, {}, io.popen
  -- CHECK FOR UPDATE FOLDER
  local test = util.os_capture("ls $HOME | grep update")
  if test ~= "update" then os.execute("mkdir $HOME/update") end
  -- COPY FROM USB
  local disk = util.os_capture("lsblk -o mountpoint | grep media")
  local pfile = popen("ls -p "..disk.."/norns*.tgz")
  for filename in pfile:lines() do
    local s = string.sub(filename,-10,-5)
    local n = tonumber(s)
    print("found update version: " .. n)
    if n > tonumber(norns.version.update) then
      print("copying update version: " .. n)
      os.execute("cp "..filename.." $HOME/update/")
    end
  end
  -- PREPARE
  pfile = popen('ls -p $HOME/update/norns*.tgz')
  for filename in pfile:lines() do
    print(filename)
    -- extract
    os.execute("tar -xzvC $HOME/update -f "..filename)
    -- check md5
    local md5 = util.os_capture("cd $HOME/update; md5sum -c *.md5")
    print(">> "..md5)
    if string.find(md5,"OK") then
      found = true
      norns.log.post("new update found")
      -- unpack
      local file = string.sub(md5,1,-5)
      os.execute("tar -xzvC $HOME/update -f $HOME/update/"..file)
    else norns.log.post("bad update file") end
    -- delete
    os.execute("rm $HOME/update/*.tgz; rm $HOME/update/*.md5")
  end
  pfile:close()

  return found
end

function Update.run()
  local pfile = io.popen('ls -tp $HOME/update')
  local newest = 0
  for file in pfile:lines() do
    local s = string.sub(file,0,-2)
    local n = tonumber(s)
    if n then
      print("update found: "..n)
      if n > newest then newest = n end
    end
  end
  if newest > tonumber(norns.version.update) then
    print("updating with: "..newest)
    screen.clear()
    screen.move(64,32)
    screen.level(10)
    screen.text_center("running update: "..newest)
    screen.update()
    os.execute("$HOME/update/"..newest.."/update.sh")
    screen.clear()
    screen.move(64,32)
    screen.level(10)
    screen.text_center("complete. shutting down.")
    screen.update()
    os.execute("rm -rf $HOME/update/*")
    os.execute("sleep 0.5; sudo shutdown now")
  end
end

return Update
