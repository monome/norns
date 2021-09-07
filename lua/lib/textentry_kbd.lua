
--- just a separate module to store textentry keyboard callbacks
--- prevents having to deal w/ a circular dependency

local te_kbd = {
  code = nil,
  char = nil,
}

return te_kbd
