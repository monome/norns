
--- just a separate module to store textentry keyboard callbacks
--- prevents having to deal w/ a circular dependency
--- @module lib.textentry_kbd
--- @alias te_kbd

local te_kbd = {
  code = nil,
  char = nil,
}

return te_kbd
