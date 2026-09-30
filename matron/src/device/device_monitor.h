#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern void dev_monitor_init(void);
extern void dev_monitor_deinit(void);
// scan connected devices
extern int dev_monitor_scan(void);

#ifdef __cplusplus
}
#endif
