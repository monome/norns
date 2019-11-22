local m = {
  pos = 0,
  list = {},
  len = "scan" 
}

local function sort_select_tree(results)
  local t = {}
  for filename in results:gmatch("[^\r\n]+") do
    if string.match(filename,"/data/")==nil and 
      string.match(filename,"/lib/")==nil then
      table.insert(t,filename)
    end
  end

  for _,file in pairs(t) do
    local p = string.match(file,".*/")
    local n = string.gsub(file,'.lua','/')
    n = string.gsub(n,_path.code,'')
    n = string.sub(n,0,-2)
    local a,b = string.match(n,"(.+)/(.+)$") -- strip similar dir/script
    if a==b and a then n = a end
    --print(file,n,p)
    table.insert(m.list,{name=n,file=file,path=p})
  end

  m.len = tab.count(m.list)
  _menu.redraw()
end


m.init = function()
  m.len = "scan" 
  m.list = {}
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
    _menu.previewfile = m.list[m.pos+1].file
    _menu.set_page("PREVIEW")
  end
end

m.enc = function(n,delta)
  if n==2 then
    m.pos = util.clamp(m.pos + delta, 0, m.len - 1)
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
        screen.text(string.upper(line))
      end
    end
  end
  screen.update()
end

return m
