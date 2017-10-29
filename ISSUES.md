# descriptors
- `oracle` module maintains command, poll, and engine descriptors. maintaining these is clunky and fragile. they also invoke mutexes to e.g. look up command strings; the architecture should't require this for every write. (one workaround option would be to pass command names from lua each time)

# polls
- *format* : polls can return either a single float value or an arbitrarily-sized byte array. in some cases it would be more efficient and convenient to support arbitrary formats, or at least arrays of floats (e.g. all I/O amplitudes in one OSC message)

# input devices
- removing a device tries to delete it twice, i think. don't foresee any ill effects, but it does print an error trying to cancel nonexistent thread.
- realizing that the whole system of switching on event types and event codes in lua, is a little ridiculous, given that we can get device code names with `libevdev_event_code_get_name()`. we can either pass the code name through the event data, or be a little dirtier and call libevdev from weaver.
  unlike ev.code numbers, ev.code *names* are distict event across event types. so lua should only need to care about the names.

# event queue optimization
there is really too much allocation and mutexing going on in the event queue.
- *allocation* : posting an event invokes `calloc` to get an `event_data` union. since it is a union, its a good candidate for object-pool model.
- *threads* : each read and write to the event queue is protected by a mutex. it is probably not a big deal, but would be better to have at least lock-free reads. (not sure how possible this really is)
