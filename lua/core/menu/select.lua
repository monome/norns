tabutil = require "tabutil"

local m = {
  pos = 0,
  list = {},
  favorites = {},
  len = "scan"
}

local function menu_table_entry(file)
  local p = string.match(file,".*/")
  local n = string.gsub(file,'.lua','/')
  n = string.gsub(n,paths.code,'')
  n = string.sub(n,0,-2)
  local a,b = string.match(n,"(.+)/(.+)$") -- strip similar dir/script
  if a==b and a then n = a end
  return {name=n,file=file,path=p}
end

local function sort_select_tree(results)
  if tab.count(m.favorites) > 0 then
    for _, entry in pairs(m.favorites) do
      table.insert(m.list,entry)
    end
    table.insert(m.list, {name="-", file=nil, path=nil})
  end

  local t = {}
  for filename in results:gmatch("[^\r\n]+") do
    if string.match(filename,"/data/")==nil and
      string.match(filename,"/lib/")==nil then
      table.insert(t,filename)
    end
  end

  for _,file in pairs(t) do
    table.insert(m.list,menu_table_entry(file))
  end

  m.len = tab.count(m.list)
  _menu.redraw()
end

local function contains(list, menu_item)
  for _, v in pairs(list) do
    if v.file == menu_item.file then
      return true
    end
  end
  return false
end

m.init = function()
  m.len = "scan"
  m.list = {}
  m.favorites = {}
  m.favorites = tabutil.load(paths.favorites)
  if m.favorites == nil then
    m.favorites = {}
    tabutil.save(m.favorites, paths.favorites)
  end
  -- weird command, but it is fast, recursive, skips hidden dirs, and sorts
  norns.system_cmd('find ~/dust/code/ -name "*.lua" | sort', sort_select_tree)
end

m.deinit = norns.none

m.key = function(n,z)
  -- back
  if n==2 and z==1 then
    _menu.set_page("HOME")
  -- select
  elseif n==3 and z==1 then
    -- return if the current "file" is the split between favorites and all scripts
    if m.list[m.pos+1].file == nil then return end
    _menu.previewfile = m.list[m.pos+1].file
    _menu.set_page("PREVIEW")
  end
end

m.enc = function(n,delta)
  if n==2 then
    m.pos = util.clamp(m.pos + delta, 0, m.len - 1)
    _menu.redraw()
  elseif n==3 then
    if delta > 0 then
      m.add_favorite()
    else
      m.remove_favorite()
    end
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()
  screen.level(15)
  if m.len == "scan" then
    screen.move(64,40)
    screen.text_center("scanning...")
  elseif m.len == 0 then
    screen.move(64,40)
    screen.text_center("no files")
  else
    for i=1,6 do
      if (i > 2 - m.pos) and (i < m.len - m.pos + 3) then
        screen.move(0,10*i)
        local line = m.list[i+m.pos-2].name
        if(i==3) then
          screen.level(15)
        else
          screen.level(4)
        end
        local is_fave = "  "
        if contains(m.favorites, m.list[i+m.pos-2]) then is_fave = "* " else is_fave = "  " end
        screen.text(is_fave .. string.upper(line))
      end
    end
  end
  screen.update()
end

m.add_favorite = function()
  -- don't add the '-' split as a favorite.
  if m.list[m.pos+1].name == '-' then
    return
  end
  if not contains(m.favorites, m.list[m.pos+1]) then
    table.insert(m.favorites, m.list[m.pos+1])
    tabutil.save(m.favorites, paths.favorites)
  end
end

m.remove_favorite = function()
  for i, v in pairs(m.favorites) do
    if v.file == m.list[m.pos+1].file then
      table.remove(m.favorites, i)
      tabutil.save(m.favorites, paths.favorites)
      return
    end
  end
end

return m
