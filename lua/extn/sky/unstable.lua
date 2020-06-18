-- bootstrap sky module
local sky = include('sky/prelude')

-- use versioned components, then unversioned components
table.insert(sky.__search, paths.extn .. 'sky/unstable/')
table.insert(sky.__search, paths.extn .. 'sky/v1/')
table.insert(sky.__search, paths.extn .. 'sky/')

-- require/include the core
sky.use('core/object')
sky.use('core/event')
sky.use('core/process')
sky.use('device/utility') -- needed by Chain
sky.use('device/virtual')

return sky

