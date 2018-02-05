require 'globals'
require 'engine'
require 'grid'
require 'hid'
require 'poll'
require 'metro'
require 'file'
require 'screen'
require 'menu'
require 'system'
require 'script'

-- resume last loaded script
sys.log.post("norns started")
sys.file.state.resume() 
