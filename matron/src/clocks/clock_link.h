#pragma once

void clock_link_init();
void clock_link_start();
void clock_link_join_session();
void clock_link_leave_session();
void clock_link_set_quantum(double quantum);
void clock_link_set_tempo(double tempo);
void clock_link_set_transport_start();
void clock_link_set_transport_stop();
void clock_link_set_start_stop_sync(bool sync_enabled);
double clock_link_get_beat();
double clock_link_get_tempo();
