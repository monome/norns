
-- setup the global sky module for devices to use without having to redundently
-- require/include the core

local function import(target, module)
  for k, v in pairs(module) do
    target[k] = v
  end
end

sky = {
  __loaded = {},
  __search = {paths.this.path, paths.code},
}

function sky.use_debug(bool)
  sky.__use_debug = bool
end

function sky.use(path, reload)
  if reload or sky.__loaded[path] == nil then
    if sky.__use_debug then
      print("searching for: " .. path)
    end
    -- look for path along search path
    for i, dir in ipairs(sky.__search) do
      local p = dir .. path .. '.lua'
      if sky.__use_debug then
        print("trying " .. p)
      end
      if util.file_exists(p) then
        print("using " .. p)
        local module = dofile(p)
        sky.__loaded[path] = module
        import(sky, module)
        return module
      end
    end
    -- didn't find anything
    error("sky.use, cannot find: " .. path)
  end

  -- return existing
  return sky.__loaded[path]
end

return sky

