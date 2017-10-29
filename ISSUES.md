# polls

- *format* : polls can return either a single float value or an arbitrarily-sized byte array. in some cases it would be more efficient and convenient to support arbitrary formats, or at least arrays of floats (e.g. all I/O amplitudes in one OSC message)


# input devices

- removing a device tries to delete it twice, i think. don't foresee any ill effects, but it does print an error trying to cancel nonexistent thread.

- realizing that the whole system of switching on event types and event codes in lua, is a little ridiculous, given that we can get device code names with `libevdev_event_code_get_name()`. we can either pass the code name through the event data, or be a little dirtier and call libevdev from weaver.
  unlike ev.code numbers, ev.code *names* are distict event across event types. so lua should only need to care about the names.