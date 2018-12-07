local function clamp(value, min, max)
  return math.min(max, (math.max(value, min)))
end

local function grid_binary_op(op, lhs, rhs)
  local result = { grid = {}, width = lhs.width, height = lhs.height }

  for row = 1, lhs.height do
    result.grid[row] = {}
    for col = 1, lhs.width do
      result.grid[row][col] = clamp(op(lhs.grid[row][col], rhs.grid[row][col]), 0, 15)
    end
  end

  setmetatable(result, getmetatable(lhs))
  return result
end

local Buffer = {
  __add = function(lhs, rhs)
    return grid_binary_op(function(a, b) return a + b end, lhs, rhs)
  end,
  __sub = function(lhs, rhs)
    return grid_binary_op(function(a, b) return a - b end, lhs, rhs)
  end,
  __bor = function(lhs, rhs)
    return grid_binary_op(function(a, b) return math.max(a, b) end, lhs, rhs)
  end,
  __bxor = function(lhs, rhs)
    return grid_binary_op(function(a, b) return a ~ b end, lhs, rhs)
  end,
}
Buffer.__index = Buffer

function Buffer.new(w, h)
  local b = {}
  setmetatable(b, Buffer)

  b.grid = {}
  b.width = w
  b.height = h

  for row = 1, h do
    b.grid[row] = {}
    for col = 1, w do
      b.grid[row][col] = 0
    end
  end

  return b
end

function Buffer:led_level_set(x, y, l)
  self.grid[y][x] = clamp(l, 0, 15)
end

function Buffer:led_level_row(x_offset, y, data)
  for i, l in ipairs(data) do
    self:led_level_set(x_offset + i - 1, y, data[i])
  end
end

function Buffer:led_level_col(x, y_offset, data)
  for i, l in ipairs(data) do
    self:led_level_set(x, y_offset + i - 1, data[i])
  end
end

function Buffer:led_level_all(l)
  for row = 1, self.height do
    for col = 1, self.width do
      self.grid[row][col] = l
    end
  end
end

function Buffer:render(grid)
  -- TODO: use map when it's available
  for row = 1, self.height do
    for col = 1, self.width do
      grid.led(col, row, self.grid[row][col])
    end
  end
end

function Buffer:print()
  for row = 1, self.height do
    local format = ""
    local values = {}
    for col = 1, self.width do
      format = format.."%01x"
      values[col] = self.grid[row][col]
    end
    print(string.format(format, table.unpack(values)))
  end
end

return Buffer
