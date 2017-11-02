--- script run/load management
-- @module file

print('file.lua')
require 'norns'
norns.version.file = '0.0.2'

dofile(script_dir .. '../last.lua')
print("last file loaded: " .. file_last)


--- load a script from the /scripts folder
-- @param - name file to load, no extension. leave blank to reload current file.
function file(name)
  local file_load = file_last
  if name ~= nil then
    file_load = name end
  local file_full = script_dir .. file_load .. '.lua'
  print("trying "..file_full)
  local f=io.open(file_full,"r")
  if f==nil then print "no file there"
  else
    io.close(f)
    dofile(file_full)
    file_last = name
    -- FIXME do we need to call a deinit first?
    local last=io.open(script_dir .. "../last.lua","w+")
    io.output(last)
    io.write("file_last = '" .. file_load .. "'\n")
    io.close(last) 
  end 
end
