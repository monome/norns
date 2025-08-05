-- luacheck configuration for norns

globals = {
    -- core norns globals
    "norns", "engine", "params", "clock", "grid", "arc", "midi", "softcut",
    "screen", "enc", "key", "util", "tab", "controlspec", "voice", "musicutil",
    "poll", "metro", "osc", "audio", "hid", "crow",

    -- crow globals
    "sequins", "timeline",

    -- system and utility globals
    "_menu", "_norns", "_path", "_startup", "cleanup", "coroutine", "debug",
    "include", "inf", "mix", "package", "paramset", "paths", "redraw", "string",
    "wifi",

    -- sky extn globals
    "sky",
}

-- for now, ignore common warnings in norns framework code:
ignore = {
    "131", -- unused implicitly defined global variable
    "211", -- unused local variables
    "212", -- unused arguments
    "213", -- unused loop variables
    "241", -- local variable is mutated but never accessed
    "311", -- value assigned to a local variable is unused
    "411", -- redefining a local variable
    "412", -- redefining an argument
    "421", -- shadowing a local variable
    "431", -- shadowing an upvalue
    "432", -- shadowing an upvalue argument
    "611", -- a line consists of nothing but whitespace
    "612", -- a line contains trailing whitespace
    "614", -- trailing whitespace in a comment
    "631"  -- line is too long
}

-- allow scripts to define global constants
allow_defined_top = true

-- skip linting core norns files and sky extension
exclude_files = {
    "lua/core/",
    "lua/lib/",
    "lua/extn/sky/"
}
