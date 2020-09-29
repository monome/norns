#pragma once

/*
 *  MIDI PARSER based from:
 * 
 * FluidSynth https://github.com/FluidSynth/fluidsynth/
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 * 
 *  midi_event_type was based from fluid_midi_event_type
 *  midi_event_t was based from fluid_midi_event_t
 *  midi_parser_t was based from fluid_midi_parser_t 
 *  All the above originals can be found at: https://github.com/FluidSynth/fluidsynth/blob/master/src/midi/fluid_midi.h
 * 
 * 
 *  midi_parser_parse was based on fluid_midi_parser_parse 
 *  midi_event_length was based on fluid_midi_event_length
 *  All the above originals can be found at: https://github.com/FluidSynth/fluidsynth/blob/master/src/midi/fluid_midi.c
 *   
 */

/**< Maximum size of MIDI parameters/data (largest is SYSEX data) */
#define MIDI_PARSER_MAX_DATA_SIZE 1024

enum midi_event_type {
    /* channel messages */
    NOTE_OFF = 0x80,
    NOTE_ON = 0x90,
    KEY_PRESSURE = 0xa0,
    CONTROL_CHANGE = 0xb0,
    PROGRAM_CHANGE = 0xc0,
    CHANNEL_PRESSURE = 0xd0,
    PITCH_BEND = 0xe0,
    /* system exclusive */
    MIDI_SYSEX = 0xf0,
    /* system common - never in midi files */
    MIDI_TIME_CODE = 0xf1,
    MIDI_SONG_POSITION = 0xf2,
    MIDI_SONG_SELECT = 0xf3,
    MIDI_TUNE_REQUEST = 0xf6,
    MIDI_EOX = 0xf7,
    /* system real-time - never in midi files */
    MIDI_SYNC = 0xf8,
    MIDI_TICK = 0xf9,
    MIDI_START = 0xfa,
    MIDI_CONTINUE = 0xfb,
    MIDI_STOP = 0xfc,
    MIDI_ACTIVE_SENSING = 0xfe,
    MIDI_SYSTEM_RESET = 0xff,
    /* meta event - for midi files only */
    MIDI_META_EVENT = 0xff
};

typedef struct _midi_event_t midi_event_t;
typedef struct _midi_parser_t midi_parser_t;

struct _midi_event_t {
    void *paramptr;           /* Pointer parameter (for SYSEX data), size is stored to param1, param2 indicates if pointer should be freed (dynamic if TRUE) */
    unsigned int param1;      /* First parameter */
    unsigned int param2;      /* Second parameter */
    unsigned char type;       /* MIDI event type */
    unsigned char channel;    /* MIDI channel */
};

struct _midi_parser_t {
    unsigned char status;           /* Identifies the type of event, that is currently received ('Noteon', 'Pitch Bend' etc). */
    unsigned char channel;          /* The channel of the event that is received (in case of a channel event) */
    unsigned int nr_bytes;          /* How many bytes have been read for the current event? */
    unsigned int nr_bytes_total;    /* How many bytes does the current event type include? */
    unsigned char data[MIDI_PARSER_MAX_DATA_SIZE]; /* The parameters or SYSEX data */
    midi_event_t event;             /* The event, that is returned to the MIDI driver. */
};

/**
 * Parse a MIDI stream one character at a time.
 * @param parser Parser instance
 * @param c Next character in MIDI stream
 * @return A parsed MIDI event or NULL if none.  Event is internal and should
 *   not be modified or freed and is only valid until next call to this function.
 * @internal Do not expose this function to the public API. It would allow downstream
 * apps to abuse fluidsynth as midi parser, e.g. feeding it with rawmidi and pull out
 * the needed midi information using the getter functions of fluid_midi_event_t.
 * This parser however is incomplete as it e.g. only provides a limited buffer to
 * store and process SYSEX data (i.e. doesn't allow arbitrary lengths)
 */
extern midi_event_t* midi_parser_parse(midi_parser_t *parser, unsigned char c);