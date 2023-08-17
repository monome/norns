
local models = {}

-- bundled w/ core
models['03000000830500006020000010010000'] = require 'gamepad_model/ibufalo_classic_usb'
models['030000005e0400008e02000014010000'] = require 'gamepad_model/xbox_360'
models['03000000790000001100000010010000'] = require 'gamepad_model/retro_controller'

-- user-local
local user_conf_dir = _path.data .. "gamepads/"
if util.file_exists(user_conf_dir) then
  local user_confs = util.scandir(user_conf_dir)
  for _, conf_file in pairs(user_confs) do
    if conf_file:sub(-#'.lua') == '.lua' then
      local conf_file_sans_ext = conf_file:gsub("%.lua", "")
      print("loading user gamepad conf: "..user_conf_dir..conf_file)
      local module_path = user_conf_dir .. conf_file_sans_ext
      local user_conf = require(module_path)
      models[user_conf.guid] = user_conf
    end
  end
end

return models
