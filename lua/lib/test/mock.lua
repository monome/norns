-- lua/lib/test/mock.lua
-- lightweight mocks/spies for testing

local M = {}

--- stub a method on an object with return values.
-- @tparam table obj target object to stub
-- @tparam string key method name to stub
-- @tparam table retvals array of return values
-- @treturn function restore function to undo the stub
function M.stub(obj, key, retvals)
  local orig = obj[key]
  obj[key] = function(_)
    return table.unpack(retvals or {})
  end
  return function()
    obj[key] = orig
  end
end

--- create a spy function that records calls.
-- optionally wraps an existing function.
-- @tparam function func optional function to wrap
-- @treturn table spy object with call tracking methods
function M.spy(func)
  local calls = {}

  local spy_obj = {}

  function spy_obj.fn(...)
    local args = { ... }
    calls[#calls + 1] = { n = select('#', ...), args = args }
    if func then
      return func(table.unpack(args, 1, select('#', ...)))
    end
  end

  -- helper methods
  function spy_obj.called(n)
    return n == nil and #calls > 0 or #calls == n
  end

  function spy_obj.args(i)
    return calls[i] and calls[i].args
  end

  function spy_obj.call_count()
    return #calls
  end

  function spy_obj.reset()
    calls = {}
  end

  -- make it callable like a function
  setmetatable(spy_obj, {
    __call = function(self, ...)
      return self.fn(...)
    end
  })

  return spy_obj
end

--- stub multiple methods on a table.
-- @tparam table t target table to stub
-- @tparam table defs method definitions (keys are method names, values are return values)
-- @treturn function restore function to undo all stubs
function M.stub_table(t, defs)
  local undo = {}
  for k, v in pairs(defs) do
    undo[#undo + 1] = M.stub(t, k, type(v) == "table" and v or { v })
  end
  return function()
    for i = #undo, 1, -1 do
      undo[i]()
    end
  end
end

return M
