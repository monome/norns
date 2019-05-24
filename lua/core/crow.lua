local util = require 'util'

local function tostringwithquotes(s)
  return "'"..tostring(s).."'"
end

function ret_cv(...)
  crow.input.receive(...)
end

function change(...)
  crow.input.receive(...)
end

norns.crow = {}

norns.crow.add = function(id, name, dev)
  print(">>>>>> norns.crow.add / " .. id .. " / " .. name)
  norns.crow.dev = dev
end

norns.crow.remove = function(id)
  print(">>>>>> norns.crow.remove " .. id)
end

norns.crow.event = function(id, line)
  line = string.sub(line,1,-2) -- strip newline
  if util.string_starts(line,"^^") == true then
    line = string.sub(line,3)
    assert(load(line))()
    --print(line)
  else
    crow.receive(line)
  end
end


norns.crow.send = function(cmd)
  _norns.crow_send(norns.crow.dev,cmd)  
end

-- ----

local crow = {}

function crow.send(cmd)
  print("crow send: "..cmd)
  _norns.crow_send(norns.crow.dev,cmd)  
end

function crow.receive(msg)
  print("crow receive: "..msg)
end


crow.input = {}

function crow.input.query(n)
  crow.send("get_cv("..n..")")
end

function crow.input.mode(...)
  local arg = {...}
  local n = arg[1]
  local args = ''
  for i=2,#arg do
    if type(arg[i]) == "string" then
      args = args .. tostringwithquotes(arg[i]) .. ","
    else
      args = args .. arg[i] .. ","
    end
  end
  args = string.sub(args,0,-2)
  crow.send("input["..n.."].mode("..args..")")
end

function crow.input.receive(n, v)
  print("crow input receive: "..n.." "..v)
end


function crow.output(n,v)
  crow.send("output["..n.."].volts="..v)
end


return crow

