#include "page.h"
#include "pages.h"

struct page *pages[NUM_PAGES];
struct page *page_cur;
int page_id_cur;

char *page_title[NUM_PAGES] = {
  "---  matron  --",
  "---  crone  ---",
  "--  matron log  --",
  "--  crone log  ---",
};

int page_nrows = 512;
int page_ncols = 80;

WINDOW *win_stat;

void page_select(int i) {
  page_id_cur = i;
  page_cur = pages[i];
  top_panel(page_cur->panel);
  mvwprintw(win_stat, 0, 0, page_title[i]);
  wnoutrefresh(win_stat);
  page_refresh(page_cur);
  doupdate();
}

void pages_init(int nrows, int ncols) {
  // status bar
  win_stat = newwin(1, ncols - 1, 0, 0);
  wbkgd(win_stat, A_STANDOUT);

  // extra space at bottom for cmd + sep
  nrows -= 3;
  page_ncols = ncols;

  for(int i = 0; i < NUM_PAGES; i++) {
    pages[i] = page_new(page_nrows, page_ncols, 0, 1, ncols - 1, nrows - 2);
  }

  page_select(0);
}

void pages_deinit(void) {
  for(int i = 0; i < NUM_PAGES; i++) {
    page_free(pages[i]);
  }
}

void page_append_selected(char *str) {
  page_print(page_cur, str);
}

void page_append(int i, char *str) {
  page_print(pages[i], str);
}

void pages_show_key(int k) {
  mvwprintw(win_stat, 0, page_ncols - 9, "%08x",k);
  wnoutrefresh(win_stat);
}

void page_cycle(void) {
  switch(page_id_cur) {
  case PAGE_MATRON:
    page_select(PAGE_CRONE);
    break;
  case PAGE_CRONE:
    page_select(PAGE_MATRON);
    break;
  case PAGE_MATRON_LOG:
    page_select(PAGE_CRONE_LOG);
    break;
  case PAGE_CRONE_LOG:
    page_select(PAGE_MATRON_LOG);
    break;
  default:
    page_select(PAGE_MATRON);
  }
}

void page_switch(void) {
  switch(page_id_cur) {
  case PAGE_MATRON:
    page_select(PAGE_MATRON_LOG);
    break;
  case PAGE_CRONE:
    page_select(PAGE_CRONE_LOG);
    break;
  case PAGE_MATRON_LOG:
    page_select(PAGE_MATRON);
    break;
  case PAGE_CRONE_LOG:
    page_select(PAGE_CRONE);
    break;
  default:
    page_select(PAGE_MATRON_LOG);
  }
}

int page_id(void) {
  return page_id_cur;
}

void pages_print_greeting(void) {
  page_print(pages[0], "MAIDEN\n");
  page_print_version(pages[0]);
}
