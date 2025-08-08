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

-- allow scripts to define global constants
allow_defined_top = true

exclude_files = {
    "lua/extn/sky/" -- exclude all sky files for now
}

-- core
files["lua/core/arc.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "412", -- redefining an argument
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "614", -- line comment not preceded by a space
        "631", -- line is too long
    },
}

files["lua/core/audio.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "614", -- line comment not preceded by a space
    },
}

files["lua/core/clock.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "612", -- line ending with single carriage return
        "613", -- inconsistent indentation
        "631", -- line is too long
    },
}

files["lua/core/config.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "631", -- line is too long
    },
}

files["lua/core/controlspec.lua"] = {
    ignore = {
        "113", -- accessing undefined variable
        "211", -- unused local variable
        "212", -- unused argument
        "214", -- unused label
        "411", -- redefining local variable
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/core/crow.lua"] = {
    ignore = {
        "111", -- setting new global variable
        "113", -- accessing undefined variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "432", -- shadowing a global variable
        "614", -- line comment not preceded by a space
    },
}

files["lua/core/crow/public.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "611", -- trailing whitespace
    },
}

files["lua/core/crow/quote.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "421", -- shadowing a local variable
    },
}

files["lua/core/encoders.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
    },
}

files["lua/core/engine.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "612", -- line ending with single carriage return
        "631", -- line is too long
    },
}

files["lua/core/gamepad.lua"] = {
    ignore = {
        "111", -- setting new global variable
        "113", -- accessing undefined variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "412", -- redefining an argument
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/core/gamepad_model/*"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/core/grid.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "412", -- redefining an argument
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "631", -- line is too long
    },
}

files["lua/core/help.lua"] = {
    ignore = {
        "131", -- accessing a field of a global variable
        "211", -- unused local variable
        "212", -- unused argument
        "631", -- line is too long
    },
}

files["lua/core/hid.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "412", -- redefining an argument
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/core/hid_device_class.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "631", -- line is too long
    },
}

files["lua/core/hid_events.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
    },
}

files["lua/core/hook.lua"] = {
    ignore = {
        "111", -- setting new global variable
        "113", -- accessing undefined variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
    },
}

files["lua/core/keymap/*"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/core/keyboard.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "412", -- redefining an argument
        "421", -- shadowing a local variable
        "612", -- line contains trailing whitespace
        "631", -- line is too long
    },
}

files["lua/core/menu.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "214", -- unused label
        "411", -- redefining local variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "631", -- line is too long
    },
}

files["lua/core/menu/devices.lua"] = {
    ignore = {
        "111", -- setting new global variable
        "113", -- accessing undefined variable
        "211", -- unused local variable
        "212", -- unused argument
        "431", -- shadowing a local variable which is an upvalue
        "581", -- negation of a relational operator
    },
}

files["lua/core/menu/display.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "631", -- line is too long
    },
}

files["lua/core/menu/home.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "412", -- redefining an argument
        "421", -- shadowing a local variable
        "611", -- line contains only whitespace
        "612", -- line contains trailing whitespace
        "631", -- line is too long
    },
}

files["lua/core/menu/mix.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/core/menu/mods.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "631", -- line is too long
    },
}

files["lua/core/menu/params.lua"] = {
    ignore = {
        "111", -- setting new global variable
        "113", -- accessing undefined variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "422", -- shadowing an upvalue
        "431", -- shadowing a local variable which is an upvalue
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "631", -- line is too long
    },
}

files["lua/core/menu/preview.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/core/menu/reset.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
    },
}

files["lua/core/menu/restart.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
    },
}

files["lua/core/menu/select.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/core/menu/settings.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/core/menu/sleep.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "421", -- shadowing a local variable
    },
}

files["lua/core/menu/system.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/core/menu/tape.lua"] = {
    ignore = {
        "111", -- setting new global variable
        "113", -- accessing undefined variable
        "211", -- unused local variable
        "212", -- unused argument
    },
}

files["lua/core/menu/update.lua"] = {
    ignore = {
        "211", -- unused variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "582", -- error-prone negation
        "631", -- line is too long
    },
}

files["lua/core/menu/wifi.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/core/metro.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
    },
}

files["lua/core/midi.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "412", -- variable was previously defined as an argument
        "431", -- shadowing a local variable which is an upvalue
        "542", -- empty if statement body
        "612", -- line contains trailing whitespace
        "631", -- line is too long
    },
}

files["lua/core/mods.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "631", -- line is too long
    },
}

files["lua/core/norns.lua"] = {
    ignore = {
        "131", -- unused global variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/core/osc.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "611", -- line contains only whitespace
        "612", -- line contains trailing whitespace
    },
}

files["lua/core/params/binary.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "412", -- variable was previously defined as an argument
        "421", -- shadowing a local variable
        "612", -- line contains trailing whitespace
        "631", -- line is too long
    },
}

files["lua/core/params/control.lua"] = {
    ignore = {
        "111", -- setting new global variable
        "113", -- accessing undefined variable
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "412", -- variable was previously defined as an argument
        "631", -- line is too long
    },
}

files["lua/core/params/file.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "412", -- variable was previously defined as an argument
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/core/params/group.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "612", -- line contains trailing whitespace
    },
}

files["lua/core/params/number.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "412", -- variable was previously defined as an argument
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/core/params/option.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "412", -- variable was previously defined as an argument
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/core/params/separator.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "421", -- shadowing a local variable
    },
}

files["lua/core/params/taper.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "412", -- variable was previously defined as an argument
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/core/params/text.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "412", -- variable was previously defined as an argument
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/core/params/trigger.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "421", -- shadowing a local variable
    },
}

files["lua/core/paramset.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "631", -- line is too long
    },
}

files["lua/core/pmap.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "612", -- line contains trailing whitespace
    },
}

files["lua/core/poll.lua"] = {
    ignore = {
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "542", -- empty if statement body
    },
}

files["lua/core/screen.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "612", -- line contains trailing whitespace
        "614", -- trailing whitespace in a comment
        "631", -- line is too long
    },
}

files["lua/core/script.lua"] = {
    ignore = {
        "111", -- setting new global variable
        "113", -- accessing undefined variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/core/serial.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "614", -- trailing whitespace in a comment
    },
}

files["lua/core/softcut.lua"] = {
    ignore = {
        "111", -- setting new global variable
        "112", -- accessing undefined global variable
        "113", -- accessing undefined variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/core/startup.lua"] = {
    ignore = {
        "131", -- unused global variable
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "631", -- line is too long
    },
}

files["lua/core/state.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/core/vport.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
    },
}

files["lua/core/wifi.lua"] = {
    ignore = {
        "113", -- accessing undefined variable
        "212", -- unused argument
        "213", -- unused loop variable
        "431", -- shadowing a local variable which is an upvalue
    },
}

-- engine
files["lua/engine/polyperc.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "611", -- line contains only whitespace
        "631", -- line is too long
    },
}

files["lua/engine/polysub.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "631", -- line is too long
    },
}

-- lib
files["lua/lib/asl.lua"] = {
    ignore = {
        "131", -- accessing a field of a global variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/lib/beatclock.lua"] = {
    ignore = {
        "113", -- accessing undefined variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "631", -- line is too long
    },
}

files["lua/lib/container/defaulttable.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "421", -- shadowing a local variable
    },
}

files["lua/lib/container/defaulttable_test.lua"] = {
    ignore = {
        "131", -- accessing a field of a global variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "411", -- redefining local variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "631", -- line is too long
    },
}

files["lua/lib/container/deque.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
    },
}

files["lua/lib/container/deque_test.lua"] = {
    ignore = {
        "131", -- accessing a field of a global variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "631", -- line is too long
    },
}

files["lua/lib/container/observable.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
    },
}

files["lua/lib/container/observable_test.lua"] = {
    ignore = {
        "131", -- accessing a field of a global variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "631", -- line is too long
    },
}

files["lua/lib/container/watchtable.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "421", -- shadowing a local variable
    },
}

files["lua/lib/container/weaktable.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "412", -- redefining an argument
        "421", -- shadowing a local variable
    },
}

files["lua/lib/elca.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "631", -- line is too long
    },
}

files["lua/lib/envgraph.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "421", -- shadowing a local variable
        "611", -- trailing whitespace
        "631", -- line is too long
    },
}

files["lua/lib/er.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
    },
}

files["lua/lib/fileselect.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/lib/filtergraph.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "611", -- trailing whitespace
        "631", -- line is too long
    },
}

files["lua/lib/filters.lua"] = {
    ignore = {
        "111", -- setting new global variable
        "113", -- accessing undefined variable
        "212", -- unused argument
        "213", -- unused loop variable
    },
}

files["lua/lib/formatters.lua"] = {
    ignore = {
        "113", -- accessing undefined variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "631", -- line is too long
    },
}

files["lua/lib/graph.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "611", -- trailing whitespace
        "614", -- line comment not preceded by a space
        "631", -- line is too long
    },
}

files["lua/lib/gridbuf.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
    },
}

files["lua/lib/hotswap.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "421", -- shadowing a local variable
        "614", -- line comment not preceded by a space
    },
}

files["lua/lib/intonation.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "612", -- line ending with single carriage return
        "631", -- line is too long
    },
}

files["lua/lib/lattice.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/lib/lfo.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "614", -- line comment not preceded by a space
        "631", -- line is too long
    },
}

files["lua/lib/listselect.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/lib/musicutil.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "614", -- line comment not preceded by a space
        "631", -- line is too long
    },
}

files["lua/lib/pattern_time.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "612", -- line ending with single carriage return
        "631", -- line is too long
    },
}

files["lua/lib/reflection.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/lib/sequins.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "631", -- line is too long
    },
}

files["lua/lib/tabutil.lua"] = {
    ignore = {
        "113", -- accessing undefined variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "612", -- line ending with single carriage return
        "614", -- line comment not preceded by a space
        "631", -- line is too long
    },
}

files["lua/lib/test/luaunit.lua"] = {
    ignore = {
        "113", -- accessing undefined variable
        "131", -- accessing a field of a global variable
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "311", -- value assigned to variable is never used
        "421", -- shadowing a local variable
        "431", -- shadowing a local variable which is an upvalue
        "581", -- negation of a relational operator
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "614", -- line comment not preceded by a space
        "631", -- line is too long
    },
}

files["lua/lib/textentry.lua"] = {
    ignore = {
        "212", -- unused argument
    },
}

files["lua/lib/textentry_kbd.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "213", -- unused loop variable
        "421", -- shadowing a local variable
        "631", -- line is too long
    },
}

files["lua/lib/timeline.lua"] = {
    ignore = {
        "412", -- redefining an argument
        "614", -- line comment not preceded by a space
    },
}

files["lua/lib/ui.lua"] = {
    ignore = {
        "211", -- unused local variable
        "212", -- unused argument
        "311", -- value assigned to variable is never used
        "611", -- trailing whitespace
        "612", -- line ending with single carriage return
        "614", -- line comment not preceded by a space
        "631", -- line is too long
    },
}

files["lua/lib/util.lua"] = {
    ignore = {
        "631", -- line is too long
    },
}

files["lua/lib/voice.lua"] = {
    ignore = {
        "212", -- unused argument
        "612", -- line ending with single carriage return
    },
}
