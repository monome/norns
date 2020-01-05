-- fileselect utility
-- reroutes redraw/enc/key

local fs = {}

function fs.enter(folder, callback)
  fs.folders = {}
  fs.list = {}
  fs.display_list = {}
  fs.lengths = {}
  fs.pos = 0
  fs.depth = 0
  fs.folder = folder
  fs.callback = callback
  fs.done = false
  fs.path = nil

  if fs.folder:sub(-1,-1) ~= "/" then
    fs.folder = fs.folder .. "/"
  end

  fs.getlist()

  if norns.menu.status() == false then
    fs.key_restore = key
    fs.enc_restore = enc
    fs.redraw_restore = redraw
    key = fs.key
    enc = fs.enc
    redraw = norns.none
    norns.menu.init()
  else
    fs.key_restore = norns.menu.get_key()
    fs.enc_restore = norns.menu.get_enc()
    fs.redraw_restore = norns.menu.get_redraw()
    norns.menu.set(fs.enc, fs.key, fs.redraw)
  end
  fs.redraw()
end

function fs.exit()
  if norns.menu.status() == false then
    key = fs.key_restore
    enc = fs.enc_restore
    redraw = fs.redraw_restore
    norns.menu.init()
  else
    norns.menu.set(fs.enc_restore, fs.key_restore, fs.redraw_restore)
  end
  if fs.path then fs.callback(fs.path)
  else fs.callback("cancel") end
end


fs.getdir = function()
  local path = fs.folder
  for k,v in pairs(fs.folders) do
    path = path .. v
  end
  --print("path: "..path)
  return path
end

fs.getlist = function()
  local dir = fs.getdir()
  fs.list = util.scandir(dir)
  fs.display_list = {}
  fs.lengths = {}
  fs.len = #fs.list
  fs.pos = 0

  -- Generate display list and lengths
  for k, v in ipairs(fs.list) do
    local line = v
    local max_line_length = 128

    if string.sub(line, -1) ~= "/" then
      local _, samples, rate = audio.file_info(dir .. line)
      if samples > 0 and rate > 0 then
        fs.lengths[k] = util.s_to_hms(math.floor(samples / rate))
        max_line_length = 97
      end
    end

    line = util.trim_string_to_width(line, max_line_length)
    fs.display_list[k] = line
  end
end

fs.key = function(n,z)
  -- back
  if n==2 and z==1 then
    if fs.depth > 0 then
      --print('back')
      fs.folders[fs.depth] = nil
      fs.depth = fs.depth - 1
      fs.getlist()
      fs.redraw()
    else
      fs.done = true
    end
    -- select
  elseif n==3 and z==1 then
    if #fs.list > 0 then
      fs.file = fs.list[fs.pos+1]
      if string.find(fs.file,'/') then
        --print("folder")
        fs.depth = fs.depth + 1
        fs.folders[fs.depth] = fs.file
        fs.getlist()
        fs.redraw()
      else
        local path = fs.folder
        for k,v in pairs(fs.folders) do
          path = path .. v
        end
        fs.path = path .. fs.file
        fs.done = true
      end
    end
  elseif z == 0 and fs.done == true then
    fs.exit()
  end
end

fs.enc = function(n,d)
  if n==2 then
    fs.pos = util.clamp(fs.pos + d, 0, fs.len - 1)
    fs.redraw()
  end
end


fs.redraw = function()
  screen.clear()
  screen.font_face(1)
  screen.font_size(8)
  if #fs.list == 0 then
    screen.level(4)
    screen.move(0,20)
    screen.text("(no files)")
  else
    for i=1,6 do
      if (i > 2 - fs.pos) and (i < fs.len - fs.pos + 3) then
        local list_index = i+fs.pos-2
        screen.move(0,10*i)
        if(i==3) then
          screen.level(15)
        else
          screen.level(4)
        end
        screen.text(fs.display_list[list_index])
        if fs.lengths[list_index] then
          screen.move(128,10*i)
          screen.text_right(fs.lengths[list_index])
        end
      end
    end
  end
  screen.update()
end

return fs
