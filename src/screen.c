#include "screen.h"
#include "format-status.h"
#include "editor.h"
#include "view.h"
#include "obuf.h"
#include "cmdline.h"
#include "search.h"
#include "uchar.h"
#include "frame.h"
#include "git-open.h"
#include "path.h"
#include "input-special.h"
#include "selection.h"

void set_color(TermColor *color)
{
    TermColor tmp = *color;

    // NOTE: -2 (keep) is treated as -1 (default)
    if (tmp.fg < 0) {
        tmp.fg = editor.builtin_colors[BC_DEFAULT]->fg;
    }
    if (tmp.bg < 0) {
        tmp.bg = editor.builtin_colors[BC_DEFAULT]->bg;
    }
    buf_set_color(&tmp);
}

void set_builtin_color(enum builtin_color c)
{
    set_color(editor.builtin_colors[c]);
}

static const char *format_misc_status(Window *win)
{
    static char misc_status[32] = {'\0'};

    if (special_input_misc_status(misc_status, 31)) {
        return misc_status;
    }

    if (editor.input_mode == INPUT_SEARCH) {
        snprintf (
            misc_status,
            sizeof(misc_status),
            "[case-sensitive = %s]",
            case_sensitivity_to_string(editor.options.case_sensitive_search)
        );
    } else if (win->view->selection) {
        SelectionInfo info;
        init_selection(win->view, &info);
        if (win->view->selection == SELECT_LINES) {
            snprintf (
                misc_status,
                sizeof(misc_status),
                "[%d lines]",
                get_nr_selected_lines(&info)
            );
        } else {
            snprintf (
                misc_status,
                sizeof(misc_status),
                "[%d chars]",
                get_nr_selected_chars(&info)
            );
        }
    } else {
        return NULL;
    }
    return misc_status;
}

void update_status_line(Window *win)
{
    Formatter f;
    char lbuf[256];
    char rbuf[256];
    int lw, rw;

    sf_init(&f, win);
    f.misc_status = format_misc_status(win);
    sf_format(&f, lbuf, sizeof(lbuf), editor.options.statusline_left);
    sf_format(&f, rbuf, sizeof(rbuf), editor.options.statusline_right);

    buf_reset(win->x, win->w, 0);
    buf_move_cursor(win->x, win->y + win->h - 1);
    set_builtin_color(BC_STATUSLINE);
    lw = u_str_width(lbuf);
    rw = u_str_width(rbuf);
    if (lw + rw <= win->w) {
        // Both fit
        buf_add_str(lbuf);
        buf_set_bytes(' ', win->w - lw - rw);
        buf_add_str(rbuf);
    } else if (lw <= win->w && rw <= win->w) {
        // Both would fit separately, draw overlapping
        buf_add_str(lbuf);
        obuf.x = win->w - rw;
        buf_move_cursor(win->x + win->w - rw, win->y + win->h - 1);
        buf_add_str(rbuf);
    } else if (lw <= win->w) {
        // Left fits
        buf_add_str(lbuf);
        buf_clear_eol();
    } else if (rw <= win->w) {
        // Right fits
        buf_set_bytes(' ', win->w - rw);
        buf_add_str(rbuf);
    } else {
        buf_clear_eol();
    }
}

int print_command(char prefix)
{
    size_t i, w;
    unsigned int u;
    int x;

    // Width of characters up to and including cursor position
    w = 1; // ":" (prefix)
    i = 0;
    while (i <= editor.cmdline.pos && i < editor.cmdline.buf.len) {
        u = u_get_char(editor.cmdline.buf.buffer, editor.cmdline.buf.len, &i);
        w += u_char_width(u);
    }
    if (editor.cmdline.pos == editor.cmdline.buf.len) {
        w++;
    }
    if (w > terminal.width) {
        obuf.scroll_x = w - terminal.width;
    }

    set_builtin_color(BC_COMMANDLINE);
    i = 0;
    buf_put_char(prefix);
    x = obuf.x - obuf.scroll_x;
    while (i < editor.cmdline.buf.len) {
        BUG_ON(obuf.x > obuf.scroll_x + obuf.width);
        u = u_get_char(editor.cmdline.buf.buffer, editor.cmdline.buf.len, &i);
        if (!buf_put_char(u)) {
            break;
        }
        if (i <= editor.cmdline.pos) {
            x = obuf.x - obuf.scroll_x;
        }
    }
    return x;
}

void print_message(const char *msg, bool is_error)
{
    enum builtin_color c = BC_COMMANDLINE;
    size_t i = 0;

    if (msg[0]) {
        c = is_error ? BC_ERRORMSG : BC_INFOMSG;
    }
    set_builtin_color(c);
    while (msg[i]) {
        unsigned int u = u_get_char(msg, i + 4, &i);
        if (!buf_put_char(u)) {
            break;
        }
    }
}

void update_term_title(Buffer* UNUSED(b))
{
    // TODO: re-implement this function properly, using the `tsl` and
    // `fsl` terminfo capabilities (or equivalent) and always restore
    // the original title on exit.
    return;

    /*
    static int term_type = -1;
    char title[1024];

    if (term_type == -1) {
        const char *term = getenv("TERM");

        term_type = 0;
        if (term) {
            if (strstr(term, "xterm") || strstr(term, "rxvt")) {
                term_type = 1;
            } else if (streq(term, "screen")) {
                term_type = 2;
            }
        }
    }

    // FIXME: title must not contain control characters
    snprintf(title, sizeof(title), "%s %c dte",
        buffer_filename(b),
        buffer_modified(b) ? '+' : '-'
    );

    switch (term_type) {
    case 1:
        // xterm or compatible
        buf_escape("\033]2;");
        buf_escape(title);
        buf_escape("\007");
        break;
    case 2:
        // tmux or screen
        // NOTE: screen might need to be configured to get it working
        buf_escape("\033_");
        buf_escape(title);
        buf_escape("\033\\");
        break;
    }
    */
}

void mask_color(TermColor *color, const TermColor *over)
{
    if (over->fg != -2) {
        color->fg = over->fg;
    }
    if (over->bg != -2) {
        color->bg = over->bg;
    }
    if (!(over->attr & ATTR_KEEP)) {
        color->attr = over->attr;
    }
}

static void print_separator(Window *win)
{
    int y;

    if (win->x + win->w == terminal.width) {
        return;
    }

    for (y = 0; y < win->h; y++) {
        buf_move_cursor(win->x + win->w, win->y + y);
        buf_add_ch('|');
    }
}

void update_separators(void)
{
    set_builtin_color(BC_STATUSLINE);
    for_each_window(print_separator);
}

void update_line_numbers(Window *win, bool force)
{
    View *v = win->view;
    long lines = v->buffer->nl;
    int first, last;
    int x = win->x + vertical_tabbar_width(win);

    calculate_line_numbers(win);

    first = v->vy + 1;
    last = v->vy + win->edit_h;
    if (last > lines) {
        last = lines;
    }

    if (
        !force
        && win->line_numbers.first == first
        && win->line_numbers.last == last
    ) {
        return;
    }

    win->line_numbers.first = first;
    win->line_numbers.last = last;

    buf_reset(win->x, win->w, 0);
    set_builtin_color(BC_LINENUMBER);
    for (int i = 0; i < win->edit_h; i++) {
        int line = v->vy + i + 1;
        int w = win->line_numbers.width - 1;
        char buf[32];

        if (line > lines) {
            snprintf(buf, sizeof(buf), "%*s ", w, "");
        } else {
            snprintf(buf, sizeof(buf), "%*d ", w, line);
        }
        buf_move_cursor(x, win->edit_y + i);
        buf_add_bytes(buf, win->line_numbers.width);
    }
}

void update_git_open(void)
{
    int x = 0;
    int y = 0;
    int w = terminal.width;
    int h = terminal.height - 1;
    int max_y = git_open.scroll + h - 1;
    int i = 0;

    if (h >= git_open.files.count) {
        git_open.scroll = 0;
    }
    if (git_open.scroll > git_open.selected) {
        git_open.scroll = git_open.selected;
    }
    if (git_open.selected > max_y) {
        git_open.scroll += git_open.selected - max_y;
    }

    buf_reset(x, w, 0);
    buf_move_cursor(0, 0);
    editor.cmdline_x = print_command('/');
    buf_clear_eol();
    y++;

    for (; i < h; i++) {
        int file_idx = git_open.scroll + i;
        char *file;
        TermColor color;

        if (file_idx >= git_open.files.count) {
            break;
        }

        file = git_open.files.ptrs[file_idx];
        obuf.x = 0;
        buf_move_cursor(x, y + i);

        color = *editor.builtin_colors[BC_DEFAULT];
        if (file_idx == git_open.selected) {
            mask_color(&color, editor.builtin_colors[BC_SELECTION]);
        }
        buf_set_color(&color);
        buf_add_str(file);
        buf_clear_eol();
    }
    set_builtin_color(BC_DEFAULT);
    for (; i < h; i++) {
        obuf.x = 0;
        buf_move_cursor(x, y + i);
        buf_clear_eol();
    }
}

void update_window_sizes(void)
{
    set_frame_size(root_frame, terminal.width, terminal.height - 1);
    update_window_coordinates();
}

void update_screen_size(void)
{
    if (!term_get_size(&terminal.width, &terminal.height)) {
        if (terminal.width < 3) {
            terminal.width = 3;
        }
        if (terminal.height < 3) {
            terminal.height = 3;
        }
        update_window_sizes();
    }
}
