--- Script class
-- @module script

local Script = {}

Script.file = ''
Script.name = 'none' 
Script.errorstate = false
Script.errormsg = ''

--- run script init() function with error checking
function Script.init()
  print("# script init")
  params.name = norns.script.name
  norns.try(init, "init")
  norns.init_done()
end

--- load a script from the /scripts folder, reset LVM
-- @param filename (string) - file to load. leave blank to reload current file.
function Script.load(filename)
  print("# script load")
  if filename == nil then
    filename = Script.file end
  local filepath = script_dir .. filename
  local f=io.open(filepath,"r")
  if f==nil then
    print("file not found: "..filepath)
    Script.file = ''
  else
    io.close(f)
    Script.file = filename
  end
  if pcall(cleanup) then print("# cleanup") 
  else print("### cleanup failed") end
  Script.savestate()
  _reset_lvm()
end

--- execute script
function Script.run()
  local filename = Script.file
  local filepath = script_dir .. filename
  local status = norns.try(function() dofile(filepath) end, "script") -- do the new script
  if status == true then
    norns.log.post("loaded " .. filename) -- post to log
    Script.folder_name = string.gsub(filename,'.lua','') -- store name
    Script.name = Script.folder_name:match("[^/]*$") -- strip path from name
    norns.script.redraw = redraw -- store redraw function for context switching
    redraw = norns.none -- block redraw until Script.init

    print("# script run")
    if engine.name ~= nil then
      print("loading engine; name: " .. engine.name)
      engine.load(engine.name, Script.init)
    else
      Script.init()
    end
  else
    norns.init_done()
  end
end


--- load script metadata
-- @param filename file to load
-- @return meta table with metadata
function Script.metadata(filename)
  local meta = {}
  if filename == nil then
    filename = norns.state.script end
  local filepath = script_dir .. filename
  local f=io.open(filepath,"r")
  if f==nil then
    print("file not found: "..filepath)
  else
    io.close(f)
    for line in io.lines(filepath) do
      if util.string_starts(line,"--") then
        table.insert(meta, string.sub(line,4,-1)) 
      else return meta end
    end
  end
  return meta
end

--- signal error in script load
function Script.error(msg)
  Script.errorstate = true
  Script.errormsg = msg
end

-- read system.lua and set parameters back to stored vals
function Script.resume()
  local f = io.open(data_dir..'system.lua')
  if f ~= nil then
    io.close(f)
    dofile(data_dir..'system.lua')
  end

  if Script.file== nil or Script.file== '' then
    Script.error("no file")
    norns.init_done()
  else  
    -- resume last file
    local f = io.open(script_dir..Script.file, "r")
    if f ~= nil then
        io.close(f)
        Script.run()
    else
        Script.error("file not found")
        norns.init_done()
    end
  end

  -- restore mix state
  mix:read("system.pset")
  mix:bang() 
end


--- save current norns state to state.lua
function Script.savestate()
  -- save mix state
  mix:write("system.pset")
  -- save current file
  local fd=io.open(data_dir .. "system.lua","w+")
  io.output(fd)
  io.write("-- system state\n")
  io.write("norns.script.file = '" .. Script.file .. "'\n")
  io.close(fd)
end


return Script
