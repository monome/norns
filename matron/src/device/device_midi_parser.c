#include "device_midi_parser.h"

#include <stdio.h>

#include "../clocks/clock_midi.h"

/* Purpose:
 * Returns the length of a MIDI message. */
static int midi_event_length(unsigned char event) {
    switch(event & 0xF0)
    {
    case NOTE_OFF:
    case NOTE_ON:
    case KEY_PRESSURE:
    case CONTROL_CHANGE:
    case PITCH_BEND:
        return 3;

    case PROGRAM_CHANGE:
    case CHANNEL_PRESSURE:
        return 2;
    }

    switch(event)
    {
    case MIDI_TIME_CODE:
    case MIDI_SONG_SELECT:
    case 0xF4:
    case 0xF5:
        return 2;

    case MIDI_TUNE_REQUEST:
        return 1;

    case MIDI_SONG_POSITION:
        return 3;
    }

    return 1;
}

midi_event_t* midi_parser_parse(midi_parser_t *parser, unsigned char c) {
    midi_event_t *event;

    /* Real-time messages (0xF8-0xFF) can occur anywhere, even in the middle
     * of another message. */
    if(c >= 0xF8) {
        
        if (c == MIDI_SYNC ||
            c == MIDI_START ||
            c == MIDI_STOP)
            clock_midi_handle_message(c);

        else if (c == MIDI_SYSTEM_RESET) {
            parser->event.type = c;
            parser->status = 0; /* clear the status */
            return &parser->event;
        }

        return NULL;
    }

    /* Status byte? - If previous message not yet complete, it is discarded (re-sync). */
    if(c & 0x80) {
        /* Any status byte terminates SYSEX messages (not just 0xF7) */
        if(parser->status == MIDI_SYSEX && parser->nr_bytes > 0) {
            event = &parser->event;
            event->type = MIDI_SYSEX;
            event->paramptr = parser->data;
            event->param1 = parser->nr_bytes;
            event->param2 = 0;
        }
        else {
            event = NULL;
        }

        /* Voice category message? */
        if (c < 0xF0) {
            parser->channel = c & 0x0F;
            parser->status = c & 0xF0;

            /* The event consumes x bytes of data... (subtract 1 for the status byte) */
            parser->nr_bytes_total = midi_event_length(parser->status) - 1;

             /* 0  bytes read so far */
            parser->nr_bytes = 0;
        }
        else if (c == MIDI_SYSEX) {
            parser->status = MIDI_SYSEX;
            parser->nr_bytes = 0;
        }
        else {
            parser->status = 0;    /* Discard other system messages (0xF1-0xF7) */
        }

        return event; /* Return SYSEX event or NULL */
    }

    /* Data/parameter byte */

    /* Discard data bytes for events we don't care about */
    if(parser->status == 0) {
        return NULL;
    }

    /* Max data size exceeded? (SYSEX messages only really) */
    if(parser->nr_bytes == MIDI_PARSER_MAX_DATA_SIZE) {
        parser->status = 0; /* Discard the rest of the message */
        return NULL;
    }

    /* Store next byte */
    parser->data[parser->nr_bytes++] = c;

    /* Do we still need more data to get this event complete? */
    if(parser->status == MIDI_SYSEX || parser->nr_bytes < parser->nr_bytes_total) {
        return NULL;
    }

    /* Event is complete, return it.
     * Running status byte MIDI feature is also handled here. */
    parser->event.type = parser->status;
    parser->event.channel = parser->channel;
    parser->nr_bytes = 0; /* Reset data size, in case there are additional running status messages */

    switch(parser->status) {
    case NOTE_OFF:
    case NOTE_ON:
    case KEY_PRESSURE:
    case CONTROL_CHANGE:
    case PROGRAM_CHANGE:
    case CHANNEL_PRESSURE:
        parser->event.param1 = parser->data[0]; /* For example key number */
        parser->event.param2 = parser->data[1]; /* For example velocity */
        break;

    case PITCH_BEND:
        /* Pitch-bend is transmitted with 14-bit precision. */
        parser->event.param1 = (parser->data[1] << 7) | parser->data[0];
        break;

    default: /* Unlikely */
        return NULL;
    }

    return &parser->event;
}
