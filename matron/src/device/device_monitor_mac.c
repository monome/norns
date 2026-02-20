#include <CoreMIDI/CoreMIDI.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "device.h"
#include "device_list.h"
#include "events.h"

static char *get_endpoint_name(MIDIEndpointRef endpoint) {
    CFStringRef name = NULL;
    MIDIObjectGetStringProperty(endpoint, kMIDIPropertyDisplayName, &name);
    if (name == NULL) {
        MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &name);
    }
    if (name == NULL) {
        return strdup("Unknown MIDI Device");
    }
    CFIndex len = CFStringGetLength(name);
    CFIndex size = CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8) + 1;
    char *buf = malloc(size);
    CFStringGetCString(name, buf, size, kCFStringEncodingUTF8);
    CFRelease(name);
    return buf;
}

void dev_monitor_init(void) {
}

void dev_monitor_deinit(void) {
}

int dev_monitor_scan(void) {
    ItemCount src = MIDIGetNumberOfSources();
    ItemCount dst = MIDIGetNumberOfDestinations();
    ItemCount count = src > dst ? src : dst;

    fprintf(stderr, "dev_monitor_mac: found %lu MIDI sources, %lu destinations\n",
            (unsigned long)src, (unsigned long)dst);

    for (ItemCount i = 0; i < count; i++) {
        char path[32];
        snprintf(path, sizeof(path), "coremidi:%lu", (unsigned long)i);

        char *name = NULL;
        if (i < src) {
            MIDIEndpointRef endpoint = MIDIGetSource(i);
            name = get_endpoint_name(endpoint);
        } else if (i < dst) {
            MIDIEndpointRef endpoint = MIDIGetDestination(i);
            name = get_endpoint_name(endpoint);
        }

        if (name == NULL) {
            name = strdup("Unknown MIDI Device");
        }

        fprintf(stderr, "dev_monitor_mac: adding MIDI device %lu: %s\n",
                (unsigned long)i, name);
        dev_list_add(DEV_TYPE_MIDI, path, name, NULL);
    }

    return 0;
}
