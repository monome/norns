// stub implementations for ableton link C API used by clock_link.c

#include "abl_link.h"

volatile int64_t g_stub_micros = 0;
volatile double g_stub_tempo = 120.0;
volatile double g_stub_beat = 0.0;
volatile uint64_t g_stub_num_peers = 0;
volatile double g_stub_requested_tempo = 0.0;
volatile int g_stub_created_count = 0;
volatile bool g_stub_is_playing = false;
volatile double g_stub_last_quantum = 0.0;
volatile bool g_stub_enabled = false;
volatile bool g_stub_sync_enabled = false;
volatile int g_stub_set_tempo_call_count = 0;

abl_link abl_link_create(double bpm) {
    (void)bpm;
    ++g_stub_created_count;
    abl_link l;
    l.impl = (void *)0x1;
    return l;
}
void abl_link_destroy(abl_link link) {
    (void)link;
}
bool abl_link_is_enabled(abl_link link) {
    (void)link;
    return true;
}
void abl_link_enable(abl_link link, bool enable) {
    (void)link;
    g_stub_enabled = enable;
}
bool abl_link_is_start_stop_sync_enabled(abl_link link) {
    (void)link;
    return false;
}
void abl_link_enable_start_stop_sync(abl_link link, bool enabled) {
    (void)link;
    g_stub_sync_enabled = enabled;
}
uint64_t abl_link_num_peers(abl_link link) {
    (void)link;
    return g_stub_num_peers;
}

void abl_link_set_num_peers_callback(abl_link link, abl_link_num_peers_callback cb, void *ctx) {
    (void)link;
    (void)cb;
    (void)ctx;
}
void abl_link_set_tempo_callback(abl_link link, abl_link_tempo_callback cb, void *ctx) {
    (void)link;
    (void)cb;
    (void)ctx;
}
void abl_link_set_start_stop_callback(abl_link link, abl_link_start_stop_callback cb, void *ctx) {
    (void)link;
    (void)cb;
    (void)ctx;
}

int64_t abl_link_clock_micros(abl_link link) {
    (void)link;
    return g_stub_micros;
}

abl_link_session_state abl_link_create_session_state(void) {
    abl_link_session_state s;
    s.impl = (void *)0x2;
    return s;
}
void abl_link_destroy_session_state(abl_link_session_state state) {
    (void)state;
}
void abl_link_capture_app_session_state(abl_link link, abl_link_session_state state) {
    (void)link;
    (void)state;
}
void abl_link_commit_app_session_state(abl_link link, abl_link_session_state state) {
    (void)link;
    (void)state;
}
double abl_link_tempo(abl_link_session_state state) {
    (void)state;
    return g_stub_tempo;
}
void abl_link_set_tempo(abl_link_session_state state, double bpm, int64_t at_time) {
    (void)state;
    (void)at_time;
    g_stub_requested_tempo = bpm;
    g_stub_set_tempo_call_count++;
}
double abl_link_beat_at_time(abl_link_session_state state, int64_t micros, double quantum) {
    (void)state;
    (void)micros;
    g_stub_last_quantum = quantum;
    return g_stub_beat;
}
bool abl_link_is_playing(abl_link_session_state state) {
    (void)state;
    return g_stub_is_playing;
}
void abl_link_set_is_playing(abl_link_session_state state, bool is_playing, int64_t at_time) {
    (void)state;
    g_stub_is_playing = is_playing;
    (void)at_time;
}
void abl_link_request_beat_at_start_playing_time(abl_link_session_state state, double beat, double quantum) {
    (void)state;
    (void)beat;
    (void)quantum;
}
