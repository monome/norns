#pragma once

/*
 * a scrolling, paneled page of text in ncurses
 */

#include <ncurses.h>
#include <panel.h>

struct page {
  WINDOW *pad;
  PANEL *panel;
  int rows; // size
  int cols;
  int row;  // text offset
  int col;
  int x;    // render location on screen
  int y;
  int w;    // screen dimensions
  int h;
};

extern struct page *page_new(int rows, int cols, int x, int y, int w, int h);
extern void page_free(struct page *page);

// append some text to the page; return new line number
extern int page_print(struct page *page, const char *txt);
// refresh; no output
extern void page_refresh(struct page *page);
// scroll to an arbitrary position and render
extern void page_scroll(struct page *page, int row, int col);
// scroll up/down and render
extern void page_scroll_up(struct page *page);
extern void page_scroll_down(struct page *page);
extern void page_scroll_forward(struct page *page);
extern void page_scroll_back(struct page *page);
// scroll to bottom and render; return new line number
extern int page_scroll_bottom(struct page *page);
// print the build version
extern int page_print_version(struct page *page);
