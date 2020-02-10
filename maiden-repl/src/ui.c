/*
 * ui.c
 *
 * indebted to https://github.com/ulfalizer/readline-and-ncurses
 */

#include <assert.h>
#include <locale.h>
#include <panel.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#include "io.h"
#include "page.h"
#include "pages.h"
#include "ui.h"

pthread_mutex_t exit_lock;

//--------------------
//---- defines
#define max(a, b)     \
  ({ typeof(a)_a = a; \
     typeof(b)_b = b; \
     _a > _b ? _a : _b; })

// call (most) ncurses functions with hard error checking
#define CHECK(fn)                               \
  do {                                          \
    if (fn() == ERR) {	                        \
      fail_exit(#fn "() failed");}}             \
  while (false)

#define CHECKV(fn, ...)                         \
  do {                                          \
    if (fn(__VA_ARGS__) == ERR) {	        \
      fail_exit(#fn "() failed");}}             \
  while (false)

//---------------------
//---- static variables

//--- flags
static bool visual_mode = false;
static bool should_exit = false;
static bool input_avail = false;

//--- windows
static WINDOW *sep_win;
static WINDOW *cmd_win;

//---- data
static unsigned char input;

//--------------------------------
//--- static function declarations

//--- lifecycle
static void init_ncurses(void);
static void deinit_ncurses(void);
static void init_readline(void);
static void deinit_readline(void);
//---- input
static int readline_input_avail(void);
static int readline_getc(FILE *dummy);
static void forward_to_readline(char c);
static void handle_cmd(char *line);
//--- display
static void cmd_win_redisplay(bool for_resize);
static void readline_redisplay(void);
static void resize(void);

static noreturn void fail_exit(const char *msg);

//--------------------------------
//---- extern function definitions

void ui_loop(void) {
  CHECKV(clearok, curscr, TRUE);
  resize();

  while (1) {
    if(should_exit) {
      break;
    }
    int c = wgetch(cmd_win);
    pages_show_key(c);
    //	printf("%08x\n", c);

    switch(c) {
    case KEY_RESIZE:
      resize();
      break;
    case '\f': // ctl-L : manual refresh
      CHECKV(clearok, curscr, TRUE);
      resize();
      break;
    case '\t':
      page_cycle();
      break;
    case 0x5a: // shift+tab
      page_switch();
      break;
    default:
      forward_to_readline(c);
    } /* switch */
  }

  mvwprintw(cmd_win, 0, 0, "EXITING!");
  ui_deinit();
}

void ui_init(void) {
  // FIXME(?) we could add all the stuff for multibyte support...
  // assume default encoding for now...
  /* if (setlocale(LC_ALL, "") == NULL) */
  /*    fail_exit("Failed to set locale attributes from environment"); */
  init_ncurses();
  init_readline();
}

void ui_deinit(void) {
  pthread_mutex_lock(&exit_lock);
  deinit_ncurses();
  deinit_readline();
  pages_deinit();
  pthread_mutex_unlock(&exit_lock);
}

static void page_line(int i, char *str) {
  pthread_mutex_lock(&exit_lock);
  if(!should_exit)  {
    page_append(i, str);
    if( page_id() == i) { 
      doupdate();
    }
  }
  pthread_mutex_unlock(&exit_lock);
}

void ui_crone_line(char *str) {
  page_line(PAGE_CRONE, str);
}

void ui_matron_line(char *str) {
  // FIXME: sloppy way to handle this
  if(strcmp(str, " <ok>\n") == 0) {
    mvwprintw(sep_win, 0, 0, "<ok>");
  } else if(strcmp(str, " <incomplete>\n") == 0) {
    mvwprintw(sep_win, 0, 0, "<incomplete>");
  } else {
    mvwprintw(sep_win, 0, 0, "              ");
    page_line(PAGE_MATRON, str);
  }
  wrefresh(sep_win);
}

//-------------------------------
//--- static function definitions

int readline_input_avail(void)
{
  return input_avail;
}

int readline_getc(FILE *dummy)
{
  (void)dummy;
  input_avail = false;
  return input;
}

void forward_to_readline(char c)
{
  input = c;
  input_avail = true;
  rl_callback_read_char();
}

void handle_cmd(char *line)
{
  if(strlen(line) == 1) {
    if (line[0] == 'q') {
      should_exit = true;
    }
  }
  else {
    if (*line != '\0') {
      add_history(line);
    }
    switch( page_id() ) {
    case PAGE_MATRON:
      io_send_line(IO_MATRON, line);
      break;
    case PAGE_CRONE:
      io_send_line(IO_CRONE, line);
      break;
    }
    free(line);
  }
}

void cmd_win_redisplay(bool for_resize)
{
  size_t cursor_col = rl_point;

  CHECKV(werase, cmd_win);

  // FIXME: error check would fail when command string is wider than window
  mvwprintw(cmd_win, 0, 0, "%s%s", rl_display_prompt, rl_line_buffer);
  if (cursor_col >= (size_t)COLS) {
    // hide the cursor if it is outside the window
    curs_set(0);
  } else {
    CHECKV(wmove, cmd_win, 0, cursor_col);
    curs_set(2);
  }
  if (for_resize) {
    CHECKV(wnoutrefresh, cmd_win);
  } else {
    CHECKV(wrefresh, cmd_win);
  }
}

void readline_redisplay(void)
{
  cmd_win_redisplay(false);
}

void resize(void)
{
  if (LINES >= 3) {
    CHECKV(wresize, sep_win, 1, COLS);
    CHECKV(wresize, cmd_win, 1, COLS);

    CHECKV(mvwin, sep_win, LINES - 2, 0);
    CHECKV(mvwin, cmd_win, LINES - 1, 0);
  }

  // batch refreshes and commit them with doupdate()
  // FIXME: resize the page pads
  CHECKV(wnoutrefresh, sep_win);
  cmd_win_redisplay(true);
  CHECK(doupdate);
}

void init_ncurses(void)
{
  if (initscr() == NULL) {
    fail_exit("failed to initialize ncurses");
  }
  visual_mode = true;

  CHECK(cbreak);
  CHECK(noecho);
  CHECK(nonl);
  CHECKV(intrflush, NULL, FALSE);

  curs_set(1);

  int nrows, ncols;
  getmaxyx(stdscr, nrows, ncols);
  sep_win = newwin(1, ncols, nrows - 2, 0);
  cmd_win = newwin(1, ncols, nrows - 1, 0);

  //nb: no keypad() because we don't want preprocessing before readline
  //  CHECK(keypad, cmd_win, TRUE);

  pages_init(nrows, ncols);

  CHECKV(wbkgd, sep_win, A_STANDOUT);
  CHECKV(wrefresh, sep_win);

  pages_print_greeting();
}

void deinit_ncurses(void)
{
  CHECKV(delwin, sep_win);
  CHECKV(delwin, cmd_win);
  CHECK(endwin);
  visual_mode = false;
}

void init_readline(void)
{
  // disable completion. (FIXME?)
  if (rl_bind_key('\t', rl_insert) != 0) {
    fail_exit("invalid key passed to rl_bind_key()");
  }

  // let ncurses do all terminal and signal handling
  rl_catch_signals = 0;
  rl_catch_sigwinch = 0;
  rl_deprep_term_function = NULL;
  rl_prep_term_function = NULL;

  // prevent readline from setting LINES, COLS
  rl_change_environment = 0;

  // handle input by manually feeding characters to readline
  rl_getc_function = readline_getc;
  rl_input_available_hook = readline_input_avail;
  rl_redisplay_function = readline_redisplay;

  rl_callback_handler_install("", handle_cmd);
}

void deinit_readline(void)
{
 rl_callback_handler_remove();
}

noreturn void fail_exit(const char *msg)
{
  if (visual_mode) {
    endwin();
  }
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}
