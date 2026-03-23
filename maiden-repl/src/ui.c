/*
 * ui.c
 *
 * indebted to https://github.com/ulfalizer/readline-and-ncurses
 */

#include <assert.h>
#include <panel.h>
#include <pthread.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#include "io.h"
#include "msg_queue.h"
#include "pages.h"
#include "ui.h"

pthread_mutex_t exit_lock;

//------------------------------------------------------------------------------
//---- defines -----------------------------------------------------------------

#define max(a, b) \
    ({ typeof(a)_a = a; \
     typeof(b)_b = b; \
     _a > _b ? _a : _b; })

// call (most) ncurses functions with hard error checking
#define CHECK(fn)                       \
    do {                                \
        if (fn() == ERR) {              \
            fail_exit(#fn "() failed"); \
        }                               \
    } while (false)

#define CHECKV(fn, ...)                 \
    do {                                \
        if (fn(__VA_ARGS__) == ERR) {   \
            fail_exit(#fn "() failed"); \
        }                               \
    } while (false)

//------------------------------------------------------------------------------
//---- variables ---------------------------------------------------------------

//--- flags
static bool visual_mode = false;
static bool should_exit = false;
static bool input_avail = false;

//--- windows
static WINDOW *sep_win;
static WINDOW *cmd_win;

//---- data
static unsigned char input;

// maps page id → socket id
// IO_COUNT means no socket (log pages)
static const int page_sock[NUM_PAGES] = {
    [PAGE_MATRON] = IO_MATRON,
    [PAGE_SC] = IO_SC,
    [PAGE_MATRON_LOG] = IO_COUNT,
    [PAGE_SC_LOG] = IO_COUNT,
};

//------------------------------------------------------------------------------
//---- function declarations ---------------------------------------------------

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

//------------------------------------------------------------------------------
//---- function definitions ----------------------------------------------------

// strip trailing whitespace/newlines in place, return new length
static size_t rstrip(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == ' ')) {
        str[--len] = '\0';
    }
    return len;
}

// append a line to a page pad with a trailing newline
static void page_appendln(int id, const char *str) {
    page_append(id, str);
    page_append(id, "\n");
}

// handle matron status tags (<ok>, <incomplete>).
// updates the status bar and appends the tag to the page. returns true if the
// message contained a status tag.
static bool handle_matron_status(int id, char *str) {
    char *ok_pos = strstr(str, "<ok>");
    char *inc_pos = strstr(str, "<incomplete>");

    if (!ok_pos && !inc_pos) {
        // no status tag — clear status bar
        mvwprintw(sep_win, 0, 0, "%-12s", "");
        wrefresh(sep_win);
        return false;
    }

    const char *tag = ok_pos ? "<ok>" : "<incomplete>";
    char *tag_pos = ok_pos ? ok_pos : inc_pos;

    mvwprintw(sep_win, 0, 0, "%-12s", tag);
    wrefresh(sep_win);

    // content before the tag (e.g. "test\n<ok>")
    if (tag_pos > str) {
        *tag_pos = '\0';
        if (rstrip(str) > 0) {
            page_appendln(id, str);
        }
    }

    page_appendln(id, tag);
    free(str);
    return true;
}

// drain queued messages from rx threads and render them.
// must only be called from the UI thread.
static void ui_drain_messages(void) {
    int id;
    char *str;
    bool did_work = false;

    while (msg_queue_pop(&id, &str)) {
        did_work = true;

        if (rstrip(str) == 0) {
            free(str);
            continue;
        }

        if (id == PAGE_MATRON && handle_matron_status(id, str)) {
            continue;
        }

        page_appendln(id, str);
        free(str);
    }

    if (did_work) {
        doupdate();
        cmd_win_redisplay(false);
    }
}

void ui_loop(void) {
    CHECKV(clearok, curscr, TRUE);
    resize();

    // non-blocking wgetch so we can periodically drain the message queue.
    wtimeout(cmd_win, 50);

    while (1) {
        if (should_exit) {
            break;
        }
        int c = wgetch(cmd_win);

        if (c == ERR) {
            // timeout — drain queued rx output and render
            ui_drain_messages();
            continue;
        }

        pages_show_key(c);

        switch (c) {
        case KEY_RESIZE:
            resize();
            break;
        case '\f': // ctl-L : manual refresh
            CHECKV(clearok, curscr, TRUE);
            resize();
            break;
        case '\t':
            page_cycle();
            cmd_win_redisplay(false);
            break;
        case '\x1b': {
            // special keys (arrows, shift+tab) arrive as multi-byte sequences
            // starting with ESC. peek ahead to catch shift+tab ourselves and
            // forward everything else to readline for history/editing.
            wtimeout(cmd_win, 10);
            int c2 = wgetch(cmd_win);
            if (c2 == '[') {
                int c3 = wgetch(cmd_win);
                if (c3 == 'Z') {
                    page_switch();
                    cmd_win_redisplay(false);
                } else {
                    forward_to_readline('\x1b');
                    forward_to_readline('[');
                    if (c3 != ERR)
                        forward_to_readline(c3);
                }
            } else {
                forward_to_readline('\x1b');
                if (c2 != ERR)
                    forward_to_readline(c2);
            }
            wtimeout(cmd_win, 50);
            break;
        }
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
    msg_queue_free();
    deinit_ncurses();
    deinit_readline();
    pages_deinit();
    pthread_mutex_unlock(&exit_lock);
}

void ui_sc_line(const char *str) {
    msg_queue_push(PAGE_SC, str);
}

void ui_matron_line(const char *str) {
    msg_queue_push(PAGE_MATRON, str);
}

//------------------------------------------------------------------------------
//---- static function definitions ---------------------------------------------

int readline_input_avail(void) {
    return input_avail;
}

int readline_getc(FILE *dummy) {
    (void)dummy;
    input_avail = false;
    return input;
}

void forward_to_readline(char c) {
    input = c;
    input_avail = true;
    rl_callback_read_char();
}

void handle_cmd(char *line) {
    if (line == NULL) {
        return;
    }
    if (strlen(line) == 1) {
        if (line[0] == 'q') {
            should_exit = true;
        }
    } else {
        int pid = page_id();

        if (*line != '\0') {
            add_history(line);

            // echo input to the output pad before sending
            page_append(pid, ">> ");
            page_append(pid, line);
            page_append(pid, "\n");
        }

        int sock = page_sock[pid];
        if (sock != IO_COUNT) {
            io_send_line(sock, line);
        }
        free(line);
    }
}

void cmd_win_redisplay(bool for_resize) {
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

void readline_redisplay(void) {
    cmd_win_redisplay(false);
}

void resize(void) {
    if (LINES >= 3) {
        CHECKV(wresize, sep_win, 1, COLS);
        CHECKV(wresize, cmd_win, 1, COLS);

        CHECKV(mvwin, sep_win, LINES - 2, 0);
        CHECKV(mvwin, cmd_win, LINES - 1, 0);
    }

    // resize page pads to match new terminal dimensions
    pages_resize(LINES, COLS);

    // batch refreshes and commit with doupdate()
    CHECKV(wnoutrefresh, sep_win);
    cmd_win_redisplay(true);
    CHECK(doupdate);
}

void init_ncurses(void) {
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

    // nb: no keypad() because we don't want preprocessing before readline
    //   CHECK(keypad, cmd_win, TRUE);

    pages_init(nrows, ncols);

    CHECKV(wbkgd, sep_win, A_STANDOUT);
    CHECKV(wrefresh, sep_win);

    pages_print_greeting();
}

void deinit_ncurses(void) {
    CHECKV(delwin, sep_win);
    CHECKV(delwin, cmd_win);
    CHECK(endwin);
    visual_mode = false;
}

void init_readline(void) {
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

void deinit_readline(void) {
    rl_callback_handler_remove();
}

noreturn void fail_exit(const char *msg) {
    if (visual_mode) {
        endwin();
    }
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}
