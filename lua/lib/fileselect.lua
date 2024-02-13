--- fileselect utility
-- @module lib.fileselect
-- reroutes redraw/enc/key

local fs = {}

function fs.enter(folder, callback, filter_string)
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
  fs.filter = filter_string and filter_string or "all"
  fs.previewing = nil
  fs.previewing_timeout_counter = nil

  if fs.folder:sub(-1, -1) ~= "/" then
    fs.folder = fs.folder .. "/"
  end

  fs.getlist()

  if norns.menu.status() == false then
    fs.key_restore = key
    fs.enc_restore = enc
    fs.redraw_restore = redraw
    fs.refresh_restore = refresh
    key = fs.key
    enc = fs.enc
    redraw = norns.none
    refresh = norns.none
    norns.menu.init()
  else
    fs.key_restore = norns.menu.get_key()
    fs.enc_restore = norns.menu.get_enc()
    fs.redraw_restore = norns.menu.get_redraw()
    fs.refresh_restore = norns.menu.get_refresh()
    norns.menu.set(fs.enc, fs.key, fs.redraw, fs.refresh)
  end
  fs.redraw()
end

function fs.exit()
  if norns.menu.status() == false then
    key = fs.key_restore
    enc = fs.enc_restore
    redraw = fs.redraw_restore
    refresh = fs.refresh_restore
    norns.menu.init()
  else
    norns.menu.set(fs.enc_restore, fs.key_restore, fs.redraw_restore, fs.refresh_restore)
  end
  if fs.path then
    fs.callback(fs.path)
  else
    fs.callback("cancel")
  end
end

function fs.pushd(dir)
  local subdir = dir:match(fs.folder .. '(.*)')
  for match in subdir:gmatch("([^/]*)/") do
    fs.depth = fs.depth + 1
    fs.folders[fs.depth] = match .. "/"
  end
  fs.getlist()
  fs.redraw()
end

fs.getdir = function()
  local path = fs.folder
  for k, v in pairs(fs.folders) do
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
  fs.visible = {}
  fs.pos = 0

  if fs.depth > 0 then
    table.insert(fs.list, 1, "../")
  end
  fs.len = #fs.list

  -- Generate display list and lengths
  for k, v in ipairs(fs.list) do
    local line = v
    local max_line_length = 128
    local display_length = "";
    local fulldir = dir .. line

    fs.visible[k] = true

    if string.sub(line, -1) ~= "/" then
      local _, samples, rate = audio.file_info(fulldir)
      -- if file is audio:
      if samples > 0 and rate > 0 then
        -- if there's no filter or we specify an "audio" or format filter:
        if fs.filter == "all" or fs.filter == "audio" or fs.filter == fulldir:match("^.+(%..+)$") then
          display_length = util.s_to_hms(math.floor(samples / rate))
          max_line_length = 97
        else         -- otherwise, do not display audio file:
          fs.visible[k] = false
          display_length = nil
        end
        -- if file is NOT audio:
      elseif fs.filter ~= "all" then
        if fs.filter == "audio" or fs.filter ~= fulldir:match("^.+(%..+)$") then
          fs.visible[k] = false
          display_length = nil
        end
      end
    end

    if fs.visible[k] then
      line = util.trim_string_to_width(line, max_line_length)
      table.insert(fs.display_list, line)
      table.insert(fs.lengths, display_length)
    end
  end
end

local function stop()
  if fs.previewing then
    fs.previewing = nil
    audio.tape_play_stop()
    fs.redraw()
  end
end

local function timeout()
  if fs.previewing_timeout_counter == nil then
    fs.previewing_timeout_counter = clock.run(function()
      clock.sleep(1)
      fs.previewing_timeout_counter = nil
    end)
  end
end

local function start()
  if fs.previewing_timeout_counter ~= nil then return end
  fs.previewing_timeout_counter = clock.run(function()
      if fs.previewing then
        stop()
        clock.sleep(0.5)
      end
      fs.previewing = fs.pos
      audio.tape_play_open(fs.getdir() .. fs.file)
      audio.tape_play_start()
      fs.redraw()
      clock.sleep(1)
      fs.previewing_timeout_counter = nil
  end)
end

fs.key = function(n, z)
  -- back
  if n == 2 and z == 1 then
    fs.done = true
    stop()
    -- select
  elseif n == 3 and z == 1 then
    stop()
    if #fs.list > 0 then
      if string.sub(fs.display_list[fs.pos + 1], -3) == '...' then
        fs.file = fs.list[fs.pos + 1]
      else
        fs.file = fs.display_list[fs.pos + 1]
      end
      if fs.file == "../" then
        fs.folders[fs.depth] = nil
        fs.depth = fs.depth - 1
        fs.getlist()
        fs.redraw()
      elseif string.find(fs.file, '/') then
        --print("folder selected")
        fs.depth = fs.depth + 1
        fs.folders[fs.depth] = fs.file
        fs.getlist()
        fs.redraw()
      else
        -- print("file selected")
        local path = fs.folder
        for k, v in pairs(fs.folders) do
          path = path .. v
        end
        fs.path = path .. fs.file
        fs.done = true
      end
    end
  elseif z == 0 and fs.done == true then
    clock.run(function()
      stop()
      clock.sleep(0.5)
      fs.exit()
      end)
  end
end

fs.enc = function(n, d)
  if n == 2 then
    fs.pos = util.clamp(fs.pos + d, 0, #fs.display_list - 1)
    fs.redraw()
  elseif n == 3 and d > 0 then
    fs.file = fs.display_list[fs.pos + 1]
    if fs.lengths[fs.pos + 1] ~= "" then
      start()
    end
  elseif n == 3 and d < 0 then
    -- always stop with left scroll
    stop()
  end
end

fs.redraw = function()
  screen.clear()
  screen.font_face(1)
  screen.font_size(8)
  if #fs.list == 0 then
    screen.level(4)
    screen.move(0, 20)
    screen.text("(no files)")
  else
    for i = 1, 6 do
      if (i > 2 - fs.pos) and (i < #fs.display_list - fs.pos + 3) then
        local list_index = i + fs.pos - 2
        screen.move(0, 10 * i)
        if (i == 3) then
          screen.level(15)
        else
          screen.level(4)
        end
        local text = fs.display_list[list_index]
        if list_index - 1 == fs.previewing then
          text = util.trim_string_to_width('* ' .. text, 97)
        end
        screen.text(text)
        if fs.lengths[list_index] then
          screen.move(128, 10 * i)
          screen.text_right(fs.lengths[list_index])
        end
      end
    end
  end
  screen.update()
end

fs.refresh = function() fs.redraw() end

return fs
