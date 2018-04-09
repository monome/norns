local Paramset = {}

Paramset.print = function(p)
  for k,v in pairs(p) do
    print(k.." = "..v:get())
  end
end

Paramset.write = function(p, filename) 
  local fd=io.open(data_dir .. filename,"w+")
  io.output(fd)
  for k,v in pairs(p) do
    io.write(k..","..v:get().."\n")
  end
  io.close(fd)
end

Paramset.read = function(p, filename)

end

Paramset.bang = function(p)
  for k,v in pairs(p) do
    v:bang()
  end
end




return Paramset
