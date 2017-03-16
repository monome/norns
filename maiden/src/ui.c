/* 
   ui.c
   
   indebted to https://github.com/ulfalizer/readline-and-ncurses
*/

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
#include "ui.h"

pthread_mutex_t exit_lock;

//--------------------
//---- defines
#define max(a, b)								\
  ({ typeof(a) _a = a;							\
	typeof(b) _b = b;  							\
	_a > _b ? _a : _b; })


// call (most) ncurses functions with hard error checking
#define CHECK(fn, ...)							\
  do											\
	if (fn(__VA_ARGS__) == ERR)					\
	  fail_exit(#fn"() failed");				\
  while (false)


//--------------------------------
//--- static function declarations

static int readline_input_avail(void);

static int readline_getc(FILE *dummy);

static void forward_to_readline(char c);

static void msg_win_redisplay(bool for_resize);

static void got_command(char *line);

static void cmd_win_redisplay(bool for_resize);

static void readline_redisplay(void);

static void resize(void);

static void init_ncurses(void);

static void deinit_ncurses(void);

static void init_readline(void);

static void deinit_readline(void);

static noreturn void fail_exit(const char *msg);

//---------------------
//---- static variables

//--- flags
static bool visual_mode = false;
static bool should_exit = false;
static bool input_avail = false;

//--- windows
static WINDOW *msg_win;
static WINDOW *sep_win;
static WINDOW *cmd_win;

//---- data
static char *msg_win_str = NULL;
static unsigned char input;

//--------------------------------
//---- extern function definitions

void ui_loop(void) {
  
  CHECK(clearok, curscr, TRUE);
  resize();
  
  while (1) {
	if(should_exit) {
	  break;
	}
	int c = wgetch(cmd_win);
		
	if (c == KEY_RESIZE) {
	  resize();
	}
	else if (c == '\f') { // ctl-L : manual refresh
	  CHECK(clearok, curscr, TRUE);
	  resize();
	}
	else {
	  forward_to_readline(c);
	}
	//	pthread_mutex_unlock(&exit_lock);
  }

  // wait for other threads/processes?
  
  mvwprintw(cmd_win, 0, 0, "EXITING!");
  ui_deinit();
  
}

void ui_init(void) {
  // FIXME(?) we could add all the stuff for multibyte support...
  // assume default encoding for now... 
  /* if (setlocale(LC_ALL, "") == NULL) */
  /* 	fail_exit("Failed to set locale attributes from environment"); */
  init_ncurses();
  init_readline();
}

void ui_deinit(void) {
  pthread_mutex_lock(&exit_lock);
  deinit_ncurses();
  deinit_readline();
  pthread_mutex_unlock(&exit_lock);
}

void ui_crone_line(char* str, size_t len) {
  // TODO
}

void ui_matron_line(char* str, size_t len) {
  pthread_mutex_lock(&exit_lock);
  if(!should_exit)  {
	size_t newlen;
	if(msg_win_str == NULL) {
	  newlen = len;
	} else {
	  newlen = len + strlen(msg_win_str);
	}
	msg_win_str = (char*)realloc(msg_win_str, newlen+1);
	strncat(msg_win_str, str, len);
	msg_win_redisplay(false);
  }
  pthread_mutex_unlock(&exit_lock);
}

//-------------------------------
//--- static function definitions

int readline_input_avail(void)
{
  return input_avail;
}

int readline_getc(FILE *dummy)
{
  input_avail = false;
  return input;
}

void forward_to_readline(char c)
{
  input = c;
  input_avail = true;
  rl_callback_read_char();
}

void msg_win_redisplay(bool for_resize)
{
  CHECK(werase, msg_win);
  CHECK(mvwaddstr, msg_win, 0, 0, msg_win_str ? msg_win_str : "");

  if (for_resize) {
	CHECK(wnoutrefresh, msg_win);
  } else {
	CHECK(wrefresh, msg_win);
  }
}

void got_command(char *line)
{
  /*
  // FIXME: this got broken somehow...
  if (line == NULL) {
	// Ctrl-D pressed on empty line
	should_exit = true;
  }
  */
  if(strlen(line) == 1 && line[0] == 'q') {
	should_exit = true;
  }
  else {
	if (*line != '\0') {
	  add_history(line);
	}
	io_send_code(line, strlen(line));
  }
}

void cmd_win_redisplay(bool for_resize)
{
  size_t prompt_width = strlen(rl_display_prompt);
  size_t cursor_col = rl_point;

  CHECK(werase, cmd_win);

  // FIXME: error check would fail when command string is wider than window 
  mvwprintw(cmd_win, 0, 0, "%s%s", rl_display_prompt, rl_line_buffer);
  if (cursor_col >= COLS) {
	// hide the cursor if it is outside the window
	curs_set(0);
  } else {
	CHECK(wmove, cmd_win, 0, cursor_col);
	curs_set(2);
  }
  if (for_resize) {
	CHECK(wnoutrefresh, cmd_win);
  } else {
	CHECK(wrefresh, cmd_win);
  }
}

void readline_redisplay(void)
{
  cmd_win_redisplay(false);
}

void resize(void)
{
  if (LINES >= 3) {
	CHECK(wresize, msg_win, LINES - 2, COLS);
	CHECK(wresize, sep_win, 1, COLS);
	CHECK(wresize, cmd_win, 1, COLS);

	CHECK(mvwin, sep_win, LINES - 2, 0);
	CHECK(mvwin, cmd_win, LINES - 1, 0);
  }

  // batch refreshes and commit them with doupdate()
  msg_win_redisplay(true);
  CHECK(wnoutrefresh, sep_win);
  cmd_win_redisplay(true);
  CHECK(doupdate);
}

void init_ncurses(void)
{
  if (initscr() == NULL) {
	fail_exit("Failed to initialize ncurses");
  }
  visual_mode = true;

  CHECK(cbreak);
  CHECK(noecho);
  CHECK(nonl);
  CHECK(intrflush, NULL, FALSE);
  //nb: no keypad() because we don't want preprocessing before readline
  
  // "very visible"
  curs_set(2);

  if (LINES >= 3) {
	msg_win = newwin(LINES - 2, COLS, 0, 0);
	sep_win = newwin(1, COLS, LINES - 2, 0);
	cmd_win = newwin(1, COLS, LINES - 1, 0);
  }
  else { // degenerate case; minimum size to avoid errors
	msg_win = newwin(1, COLS, 0, 0);
	sep_win = newwin(1, COLS, 0, 0);
	cmd_win = newwin(1, COLS, 0, 0);
  }
  if (msg_win == NULL || sep_win == NULL || cmd_win == NULL)
	fail_exit("failed to allocate windows");

  CHECK(scrollok, msg_win, TRUE);
  
  CHECK(wbkgd, sep_win, A_STANDOUT);
  CHECK(wrefresh, sep_win);
}

void deinit_ncurses(void)
{

  CHECK(delwin, msg_win);
  CHECK(delwin, sep_win);
  CHECK(delwin, cmd_win);

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

  rl_callback_handler_install("", got_command);
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

