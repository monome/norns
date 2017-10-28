# polls

- *format* : polls can return either a single float value or an arbitrarily-sized byte array. in some cases it would be more efficient and convenient to support arbitrary formats, or at least arrays of floats (e.g. all I/O amplitudes in one OSC message)


# devices

- removing a device tries to delete it twice, i think. don't think there are ill effects but it does print an error trying to cancel nonexistent thread