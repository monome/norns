-- -------------------------------
-- monome device manager

norns.monome = {}


norns.monome.add = function(id, serial, name, dev)
  -- TODO is there a better way to determine?
  if (string.find(name, "arc") == nil) then
    -- print("grid detected")
    norns.grid.add(id,serial,name,dev)
  else
    -- print("arc detected")
    norns.arc.add(id,serial,name,dev)
  end
end


norns.monome.remove = function(id)
  -- call both since we don't know if it's a grid or an arc
  -- grid/arc functions will check for existence
  norns.grid.remove(id)
  norns.arc.remove(id)
end
