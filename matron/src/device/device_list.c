#include <search.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <alloca.h>

#include "device.h"
#include "device_list.h"
#include "device_midi.h"
#include "events.h"

static int id = 0;

struct dev_node {
    struct dev_node *next;
    struct dev_node *prev;
    union dev *d;
};

struct dev_q {
    struct dev_node *head;
    struct dev_node *tail;
    size_t size;
};

struct dev_q dq;

static struct dev_node *dev_lookup_path(const char *path, struct dev_node *node_head) {
    const char *npath;
    if (node_head == NULL) {
        node_head = dq.head;
    }

    while (node_head != NULL) {
        npath = node_head->d->base.path;
        if (strcmp(path, npath) == 0) {
            return node_head;
        }
        node_head = node_head->next;
    }
    return NULL;
}

void dev_list_init(void) {
    dq.size = 0;
    dq.head = NULL;
    dq.tail = NULL;
}

union event_data *post_add_event(union dev *d, event_t event_type) {
    if (d == NULL) {
        fprintf(stderr, "dev_list_add: error allocating device data\n");
        return NULL;
    }

    struct dev_node *dn = calloc(1, sizeof(struct dev_node));

    if (dn == NULL) {
        fprintf(stderr, "dev_list_add: error allocating device queue node\n");
        free(d);
        return NULL;
    }

    d->base.id = id++;
    dn->d = d;

    insque(dn, dq.tail);
    dq.tail = dn;
    if (dq.size == 0) {
        dq.head = dn;
    }
    dq.size++;

    union event_data *ev;
    ev = event_data_new(event_type);
    return ev;
}

void dev_list_add(device_t type, const char *path, const char *name) {
    if (type < 0) {
        return;
    }

    union event_data *ev;
    union dev *d;
    unsigned int midi_port_count = 0;

    switch (type) {
    case DEV_TYPE_MIDI:
        midi_port_count = dev_midi_port_count(path);
        for (unsigned int pidx = 0; pidx < midi_port_count; pidx++) {
            d = dev_new(type, path, name, midi_port_count > 1, pidx, false);
            ev = post_add_event(d, EVENT_MIDI_ADD);
            if (ev != NULL) {
                ev->midi_add.dev = d;
                event_post(ev);
            }
        }
        return;
    case DEV_TYPE_MONOME:
        d = dev_new(type, path, name, true, 0, false);
        ev = post_add_event(d, EVENT_MONOME_ADD);
        break;
    case DEV_TYPE_HID:
        d = dev_new(type, path, name, true, 0, false);
        ev = post_add_event(d, EVENT_HID_ADD);
        break;
    case DEV_TYPE_CROW:
        d = dev_new(type, path, name, true, 0, false);
        ev = post_add_event(d, EVENT_CROW_ADD);
        break;
    default:
        fprintf(stderr, "dev_list_add(): error posting event (unknown type)\n");
        return;
    }
    if (ev != NULL) {
        ev->monome_add.dev = d;
        event_post(ev);
    }
}

static void dev_list_remove_node(struct dev_node *dn, union event_data *event_remove) {
    event_post(event_remove);

    if (dq.head == dn) {
        dq.head = dn->next;
    }
    if (dq.tail == dn) {
        dq.tail = dn->prev;
    }
    remque(dn);
    dq.size--;

    dev_delete(dn->d);
    free(dn);
}

void dev_list_remove(device_t type, const char *node) {
    struct dev_node *dn = dev_lookup_path(node, NULL);
    if (dn == NULL) {
        return;
    }
    union event_data *ev;

    switch (type) {
    case DEV_TYPE_MIDI:
        while (dn != NULL) {
            ev = event_data_new(EVENT_MIDI_REMOVE);
            ev->midi_remove.id = dn->d->base.id;
            dev_list_remove_node(dn, ev);
            dn = dev_lookup_path(node, NULL);
        }
        return;
    case DEV_TYPE_MONOME:
        ev = event_data_new(EVENT_MONOME_REMOVE);
        ev->monome_remove.id = dn->d->base.id;
        break;
    case DEV_TYPE_HID:
        ev = event_data_new(EVENT_HID_REMOVE);
        ev->hid_remove.id = dn->d->base.id;
        break;
    case DEV_TYPE_CROW:
        ev = event_data_new(EVENT_CROW_REMOVE);
        ev->crow_remove.id = dn->d->base.id;
        break;
    default:
        fprintf(stderr, "dev_list_remove(): error posting event (unknown type)\n");
        return;
    }
    dev_list_remove_node(dn, ev);
}

void list_device(snd_ctl_t *ctl, int card, int device) {
    snd_rawmidi_info_t *info;
    const char *name;
    const char *sub_name;
    int subs, subs_in, subs_out;
    int sub;
    int err;

    snd_rawmidi_info_alloca(&info);
    snd_rawmidi_info_set_device(info, device);

    snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
    err = snd_ctl_rawmidi_info(ctl, info);
    if (err >= 0)
        subs_in = snd_rawmidi_info_get_subdevices_count(info);
    else
        subs_in = 0;

    snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
    err = snd_ctl_rawmidi_info(ctl, info);
    if (err >= 0)
        subs_out = snd_rawmidi_info_get_subdevices_count(info);
    else
        subs_out = 0;

    subs = subs_in > subs_out ? subs_in : subs_out;
    if (!subs)
        return;

    for (sub = 0; sub < subs; ++sub) {
        snd_rawmidi_info_set_stream(info, sub < subs_in ?
                        SND_RAWMIDI_STREAM_INPUT :
                        SND_RAWMIDI_STREAM_OUTPUT);
        snd_rawmidi_info_set_subdevice(info, sub);
        err = snd_ctl_rawmidi_info(ctl, info);
        if (err < 0) {
            printf("cannot get rawmidi information %d:%d:%d: %s\n",
                card, device, sub, snd_strerror(err));
            return;
        }
        name = snd_rawmidi_info_get_name(info);

        sub_name = snd_rawmidi_info_get_subdevice_name(info);
        if (sub == 0 && sub_name[0] == '\0') {
            // printf("%c%c  hw:%d,%d    %s",
            //     sub < subs_in ? 'I' : ' ',
            //     sub < subs_out ? 'O' : ' ',
            //     card, device, name);
            // if (subs > 1)
            //     printf(" (%d subdevices)", subs);
            // putchar('\n');
            if ( strcmp(name,"Virtual Raw MIDI") == 0) {
                char path[32];
                sprintf(path, "/dev/snd/midiC%dD%d", card, device); 
                printf(" VIRTUAL DEVICE FOUND AT %s\n", path);
                dev_list_add(DEV_TYPE_MIDI, path, name);
            }
            break;
        } else {
            printf("%c%c  hw:%d,%d,%d  %s\n",
                sub < subs_in ? 'I' : ' ',
                sub < subs_out ? 'O' : ' ',
                card, device, sub, sub_name);
        }
    }
}

void list_card_devices(int card) {
    snd_ctl_t *ctl;
    char name[32];
    int device;
    int err;

    sprintf(name, "hw:%d", card);
    if ((err = snd_ctl_open(&ctl, name, 0)) < 0) {
        printf("cannot open control for card %d: %s", card, snd_strerror(err));
        return;
    }
    device = -1;
    for (;;) {
        if ((err = snd_ctl_rawmidi_next_device(ctl, &device)) < 0) {
            printf("cannot determine device number: %s", snd_strerror(err));
            break;
        }
        if (device < 0)
            break;
        list_device(ctl, card, device);
    }
    snd_ctl_close(ctl);
}

void dev_list_init_virtual_midi(void) {
    int card, err;

    card = -1;
    if ((err = snd_card_next(&card)) < 0) {
        printf("cannot determine card number: %s", snd_strerror(err));
        return;
    }
    if (card < 0) {
        printf("no sound card found");
        return;
    }

    do {
        list_card_devices(card);
        if ((err = snd_card_next(&card)) < 0) {
            printf("cannot determine card number: %s", snd_strerror(err));
            break;
        }
    } while (card >= 0);
}

void dev_virtual_init(void) {
    union event_data *ev;
    union dev *d;

    d = dev_new(DEV_TYPE_MIDI, "/dev/null", "virtual", false, 0, true);
    ev = post_add_event(d, EVENT_MIDI_ADD);
    if (ev != NULL) {
        ev->midi_add.dev = d;
        event_post(ev);
    }
}
