#include <stdlib.h>
#include <ncurses.h>
#include <panel.h>
#include "page.h"

//---------------------
//---- static functions

static void set_row(struct page *p, int r) {
  if(r >= p->rows) { r = p->rows - 1; }
  if(r < 0) { r = 0; }
  p->row = r;
}

static void set_col(struct page *p, int c) {
  if(c >= p->cols) { c = p->cols - 1; }
  if(c < 0) { c = 0; }
  p->col = c;
}

//---------------------
//---- extern functions

struct page *page_new(int rows, int cols, int x, int y, int w, int h) {
  struct page *p = calloc( 1, sizeof(struct page) );
  p->rows = rows;
  p->cols = cols;
  p->pad = newpad(rows, cols);
  scrollok(p->pad, TRUE);
  p->panel = new_panel(p->pad);
  p->x = x;
  p->y = y;
  p->w = w;
  p->h = h;

  return p;
}

void page_free(struct page *p) {
  if(p == NULL) { return; }
  delwin(p->pad);
  free(p);
}

int page_print(struct page *p, const char *txt) {
  wprintw(p->pad, txt);
  return page_scroll_bottom(p);
}

void page_refresh(struct page *p) {
  pnoutrefresh(p->pad, p->row, p->col, p->y, p->x, p->h, p->w);
}

void page_scroll(struct page *p, int row, int col) {
  set_row(p, row);
  set_col(p, col);
  page_refresh(p);
}

void page_scroll_up(struct page *p) {
  set_row(p, p->row - 1);
  page_refresh(p);
}

void page_scroll_down(struct page *p) {
  set_row(p, p->row + 1);
  page_refresh(p);
}

void page_scroll_forward(struct page *p) {
  set_col(p, p->col + 1);
  page_refresh(p);
}

void page_scroll_back(struct page *p) {
  set_col(p, p->col - 1);
  page_refresh(p);
}

int page_scroll_bottom(struct page *p) {
  int x, y;
  getyx(p->pad, y, x);
  (void)x;
  set_row(p, y - p->h + 2);

  page_refresh(p);
  return y;
}

int page_print_version(struct page *p) {
  wprintw(p->pad, "norns version: %d.%d.%d\n",
      0,0,0);
          //VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  //wprintw(p->pad, "git hash: %s\n", VERSION_HASH);
  return page_scroll_bottom(p);
}
