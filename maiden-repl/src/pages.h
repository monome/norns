#pragma once

enum {
  PAGE_MATRON,
  PAGE_CRONE,
  PAGE_MATRON_LOG,
  PAGE_CRONE_LOG,
  NUM_PAGES
};

extern void pages_init(int nrows, int ncols);
extern void pages_deinit(void);

extern void page_select(int i);
extern void page_append_selected(char *str);
extern void page_append(int i, char *str);

extern void pages_show_key(int k);
// switch between pages
extern void page_cycle(void);
// switch between showing output / input log
extern void page_switch(void);
// get current page index
extern int page_id(void);

extern void pages_print_greeting(void);
