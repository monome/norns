// stub header for ableton link C API used by clock_link.c
// this stub satisfies the compiler without pulling in the entire link library.

#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct abl_link {
    void *impl;
} abl_link;
typedef struct abl_link_session_state {
    void *impl;
} abl_link_session_state;

abl_link abl_link_create(double bpm);
void abl_link_destroy(abl_link link);
bool abl_link_is_enabled(abl_link link);
void abl_link_enable(abl_link link, bool enable);
bool abl_link_is_start_stop_sync_enabled(abl_link link);
void abl_link_enable_start_stop_sync(abl_link link, bool enabled);
uint64_t abl_link_num_peers(abl_link link);

typedef void (*abl_link_num_peers_callback)(uint64_t num_peers, void *context);
void abl_link_set_num_peers_callback(abl_link link, abl_link_num_peers_callback callback, void *context);

typedef void (*abl_link_tempo_callback)(double tempo, void *context);
void abl_link_set_tempo_callback(abl_link link, abl_link_tempo_callback callback, void *context);

typedef void (*abl_link_start_stop_callback)(bool is_playing, void *context);
void abl_link_set_start_stop_callback(abl_link link, abl_link_start_stop_callback callback, void *context);

int64_t abl_link_clock_micros(abl_link link);

abl_link_session_state abl_link_create_session_state(void);
void abl_link_destroy_session_state(abl_link_session_state state);
void abl_link_capture_app_session_state(abl_link link, abl_link_session_state state);
void abl_link_commit_app_session_state(abl_link link, abl_link_session_state state);
double abl_link_tempo(abl_link_session_state state);
void abl_link_set_tempo(abl_link_session_state state, double bpm, int64_t at_time);
double abl_link_beat_at_time(abl_link_session_state state, int64_t micros, double quantum);
bool abl_link_is_playing(abl_link_session_state state);
void abl_link_set_is_playing(abl_link_session_state state, bool is_playing, int64_t at_time);
void abl_link_request_beat_at_start_playing_time(abl_link_session_state state, double beat, double quantum);

#ifdef __cplusplus
}
#endif
