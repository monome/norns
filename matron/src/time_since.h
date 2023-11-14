// extremely simple CPU time measurement module, for benchmarking
// sets a single reference point, and computes time deltas since that point

#pragma once

extern void cpu_time_start();

extern unsigned long int cpu_time_get_delta_ns();

extern void wall_time_start();

extern unsigned long int wall_time_get_delta_ns();