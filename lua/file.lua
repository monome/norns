--- state management
-- @section state

norns.state = {}

norns.state.resume = function()
  dofile(script_dir .. '../state.lua')
  print("last file loaded: " .. norns.state.script)
  norns.script.load()
end

norns.state.save = function()
  local last=io.open(script_dir .. "../state.lua","w+")
  io.output(last)
  io.write("-- state\n")
  io.write("norns.state.script = '" .. norns.state.script .. "'\n")
  io.close(last)   
end


--- script managment
-- @section script

norns.script = {}
norns.script.cleanup_default = function()
   print("cleanup (default)")
end

norns.script.cleanup = norns.script.cleanup_default

norns.script.deinit = function()
    redraw = norns.blank
    key = norns.none
    enc = norns.none
end


--- load a script from the /scripts folder
-- @param filename (string) - file to load, no extension. leave blank to reload current file.
norns.script.load = function(filename)
  if filename == nil then
    filename = norns.state.script end
  local filepath = script_dir .. filename .. '.lua'
  local f=io.open(filepath,"r")
  if f==nil then 
    print("file not found: "..filepath)
  else
    io.close(f)
    norns.script.cleanup() -- cleanup the old script
    norns.script.cleanup = norns.script.cleanup_default
    norns.script.deinit()
    dofile(filepath)
    norns.state.script = filename
    norns.state.save()
    init()
  end 
end


scandir = function(directory)
    local i, t, popen = 0, {}, io.popen
    local pfile = popen('ls "'..directory..'"')
    for filename in pfile:lines() do
        i = i + 1
        t[i] = filename
    end
    pfile:close()
    return t
end

function tablelength(T)
  local count = 0
  for _ in pairs(T) do count = count + 1 end
  return count
end
