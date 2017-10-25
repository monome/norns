# poll cleanup
- *format* : polls can return either a single float value or an arbitrarily-sized byte array. in some cases it would be more efficient and convenient to support arbitrary formats, or at least arrays of floats (e.g. all I/O amplitudes in one OSC message)
- *syntax* : poll objects in lua should have methods like .start, .stop, .period. right now these are functions in top-level norns object and poll objects just hold data - clunky.

# event queue optimization
there is really too much allocation and mutexing going on in the event queue.
- *allocation* : posting an event invokes `calloc` to get an `event_data` union. since it is a union, its a good candidate for object-pool model.
- *threads* : each read and write to the event queue is protected by a mutex. it is probably not a big deal, but would be better to have at least lock-free reads. (not sure how possible this really is)

# descriptors
- `oracle` module maintains command, poll, and engine descriptors. maintaining these is clunky and fragile. they also invoke mutexes to e.g. look up command strings; the architecture should't require this for every write. (one workaround option would be to pass command names from lua each time)
