#include "buffer.h"
#include "mem.h"
#include "term.h"

#include <assert.h>
#include <stdio.h>
#include <ctype.h>

#define TAB_WIDTH 8

buffer *
buffer_from(str      name,
            str      path,
            unsigned w,
            unsigned h,
            unsigned ws,
            unsigned hs,
            linep_ar lns)
{
        buffer *b;

        b          = (buffer *)alloc(sizeof(buffer));
        b->name    = name;
        b->path    = path;
        b->size.w  = w;
        b->size.h  = h;
        b->size.ws = ws;
        b->size.hs = hs;
        b->lines   = lns;
        b->cx      = 0;
        b->cy      = 0;
        b->al      = 0;
        b->hoff    = 0;
        b->voff    = 0;

        return b;
}

static unsigned
visual_column(const str *s,
              size_t     char_idx,
              unsigned   tab_width)
{
        unsigned col = 0;
        for (size_t i = 0; i < char_idx && i < s->len; ++i) {
                if (s->chars[i] == '\t') {
                        col += tab_width - (col % tab_width);
                } else {
                        ++col;
                }
        }
        return col;
}

static unsigned
visual_width_up_to(const str *s, size_t char_idx, unsigned tab_width)
{
        return visual_column(s, char_idx, tab_width);
}

static size_t
char_index_at_visual_col(const str *s, unsigned target_col, unsigned tab_width)
{
        // Find the character index for a given visual column
        unsigned col = 0;
        for (size_t i = 0; i < s->len; ++i) {
                if (col >= target_col)
                        return i;
                if (s->chars[i] == '\t') {
                        col += tab_width - (col % tab_width);
                } else {
                        ++col;
                }
        }
        return s->len;
}

static void
adjust_cursor(buffer *b)
{
        const str *s = &b->lines.data[b->al]->txt;
        unsigned x = visual_column(s, b->cx, TAB_WIDTH);
        gotoxy(b->size.ws + (unsigned)(x > b->hoff ? x - b->hoff : 0U),
               b->size.hs + (unsigned)(b->cy - b->voff));
}

static unsigned
get_win_hight(const buffer *b)
{
        return b->size.h > 0 ? b->size.h - 1 : 0;
}

static unsigned
get_win_width(const buffer *b)
{
        return b->size.w > 0 ? b->size.w : 80;
}

static int
adjust_vscroll(buffer *b)
{
        size_t win_h = get_win_hight(b);

        if (b->cy < b->voff) {
                b->voff = b->cy;
                return 1;
        } else if (b->cy >= b->voff + win_h) {
                b->voff = b->cy - win_h + 1;
                return 1;
        }
        return 0;
}

static int
adjust_hscroll(buffer *b)
{
        const unsigned  tabw  = TAB_WIDTH;
        const str      *s     = &b->lines.data[b->al]->txt;
        unsigned        win_w = get_win_width(b);

        unsigned cursor_visual = visual_column(s, b->cx, tabw);

        if (cursor_visual < b->hoff) {
                b->hoff = cursor_visual;
                return 1;
        } else if (cursor_visual >= b->hoff + win_w) {
                b->hoff = cursor_visual - win_w + 1;
                return 1;
        }
        return 0;
}

static buffer_action
adjust_scroll(buffer *b)
{
        int res;

        res = adjust_vscroll(b);
        res = adjust_hscroll(b) || res;

        return res ? BA_REDRAW : BA_NOP;
}

static buffer_action
up(buffer *b)
{
        if (b->cy > 0) {
                const str *olds = &b->lines.data[b->al]->txt;
                unsigned desired = visual_column(olds, b->cx, TAB_WIDTH);
                --b->cy;
                --b->al;
                const str *news = &b->lines.data[b->al]->txt;
                b->cx = (unsigned)char_index_at_visual_col(news, desired, TAB_WIDTH);
        }

        if (b->cx > b->lines.data[b->al]->txt.len-1)
                b->cx = (unsigned)b->lines.data[b->al]->txt.len-1;

        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
down(buffer *b)
{
        if (b->cy < b->lines.len-1) {
                const str *olds = &b->lines.data[b->al]->txt;
                unsigned desired = visual_column(olds, b->cx, TAB_WIDTH);
                ++b->cy;
                ++b->al;
                const str *news = &b->lines.data[b->al]->txt;
                b->cx = (unsigned)char_index_at_visual_col(news, desired, TAB_WIDTH);
        }

        if (b->cx > b->lines.data[b->al]->txt.len-1)
                b->cx = (unsigned)b->lines.data[b->al]->txt.len-1;

        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
right(buffer *b)
{
        str *s = &b->lines.data[b->al]->txt;

        if (b->cx < str_len(s)-1) {
                ++b->cx;
        } else if (b->cy < b->lines.len-1) {
                ++b->cy;
                ++b->al;
                b->cx = 0;
        }

        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
left(buffer *b)
{
        if (b->cx > 0) {
                --b->cx;
        } else if (b->cy > 0) {
                --b->cy;
                --b->al;
                str *prev = &b->lines.data[b->al]->txt;
                b->cx = (unsigned)str_len(prev)-1;
        }

        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
bol(buffer *b)
{
        b->cx = 0;
        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
eol(buffer *b)
{
        str *s = &b->lines.data[b->al]->txt;
        b->cx = (unsigned)str_len(s)-1;
        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
jump_next_word(buffer *b)
{
        const line *ln;
        const str  *s;
        const char *sraw;
        int         hitchars;
        size_t      i;

        ln       = b->lines.data[b->al];
        s        = &ln->txt;
        sraw     = str_cstr(s);
        hitchars = 0;
        i        = b->cx;

        if (str_len(s) <= 0)
                return BA_NOP;

        while (i < str_len(s)) {
                if (isalnum(sraw[i]))
                        hitchars = 1;
                else if (hitchars)
                        break;
                ++i;
        }

        if (i == str_len(s))
                b->cx = (unsigned)str_len(s)-1;
        else
                b->cx = (unsigned)i;

        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
jump_prev_word(buffer *b)
{
        const line *ln;
        const str  *s;
        const char *sraw;
        int         hitchars;
        size_t      i;

        ln       = b->lines.data[b->al];
        s        = &ln->txt;
        sraw     = str_cstr(s);
        hitchars = 0;
        i        = b->cx-1;

        if (str_len(s) == 0 || b->cx == 0)
                return BA_NOP;

        while (i > 0) {
                if (isalnum(sraw[i]))
                        hitchars = 1;
                else if (hitchars)
                        break;
                --i;
        }

        b->cx = (unsigned)i;

        if (!isalnum(sraw[b->cx]))
                ++b->cx;

        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
del_word(buffer *b)
{
        line        *ln;
        str         *s;
        const char  *sraw;
        int          hitchars;
        size_t       i;

        ln       = b->lines.data[b->al];
        s        = &ln->txt;
        sraw     = str_cstr(s);
        hitchars = 0;
        i        = b->cx;

        //clear_cpy();
        while (i < str_len(s)) {
                if (sraw[i] == 10)
                        break;
                if (isalnum(sraw[i]))
                        hitchars = 1;
                else if (!isalnum(sraw[i]) && hitchars)
                        break;
                //array_append(g_cpy_buf, str_at(s, i));
                str_rm(s, i);
        }

        return adjust_scroll(b);
}

static buffer_action
prev_paragraph(buffer *b)
{
        size_t nextln;

        nextln = b->cy;

        for (int i = (int)b->cy-1; i >= 0; --i) {
                const line *l  = b->lines.data[i];
                const line *l2 = b->lines.data[i+1];
                nextln         = (size_t)i;
                if (str_len(&l->txt) == 1 && l->txt.chars[0] == '\n') {
                        if (i > 0 && l2 && l2->txt.chars[0] == '\n')
                                continue;
                        break;
                }
        }

        b->cy = (unsigned)nextln;
        b->cx = 0;
        b->al = nextln;

        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
next_paragraph(buffer *b)
{
        size_t nextln;

        nextln = b->cy;

        for (size_t i = b->cy+1; i < b->lines.len; ++i) {
                const line *l  = b->lines.data[i];
                const line *l2 = b->lines.data[i+1];
                nextln         = i;
                if (str_len(&l->txt) == 1 && l->txt.chars[0] == 10) {
                        if (i < b->lines.len && l2 && l2->txt.chars[0] == '\n')
                                continue;
                        break;
                }
        }

        b->cy = (unsigned)nextln;
        b->cx = 0;
        b->al = nextln;

        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
kill_line(buffer *b)
{
        /* if (!writable(b)) */
        /*         return; */

        line *ln;
        /* const str *s; */

        if (b->lines.len <= 0)
                return BA_NOP;

        ln = b->lines.data[b->al];
        /* s  = &ln->txt; */

        //clear_cpy();
        /* for (size_t i = 0; i < str_len(s); ++i) */
        /*         array_append(g_cpy_buf, str_at(s, i)); */

        line_destroy(ln);
        array_rm_at(b->lines, b->al);

        if (b->al > b->lines.len-1) {
                --b->al;
                --b->cy;
        }

        b->cx = 0;

        return adjust_scroll(b);
}

static buffer_action
insert_char(buffer *b, char ch, int newline_advance)
{
        /* if (!writable(b)) */
        /*         return; */

        /* b->saved = 0; */

        if (!b->lines.data) {
                char tmp[] = {'\n', 0};
                array_append(b->lines, line_from(str_from(tmp)));
                //array_append(b->lines, line_from(str_from("\n")));
        }

        str_insert(&b->lines.data[b->al]->txt, b->cx, ch);
        ++b->cx;

        if (ch == '\n') {
                const char *rest;
                line       *newln;

                rest  = str_cstr(&b->lines.data[b->al]->txt)+b->cx;
                newln = line_from(str_from(rest));

                array_insert_at(b->lines, b->al+1, newln);
                str_cut(&b->lines.data[b->al]->txt, b->cx);

                if (newline_advance) {
                        b->cx = 0;
                        ++b->cy;
                        ++b->al;
                }
        }

        //add_to_popxy(b);

        adjust_cursor(b);
        return (adjust_scroll(b) == BA_REDRAW || ch == '\n') ? BA_REDRAW : BA_XY;
}

static buffer_action
backspace(buffer *b)
{
        assert(b && 0);
        return 0;
}

// entrypoint
buffer_action
buffer_process(buffer *b)
{
        (void)visual_width_up_to;

        input_type ty;
        char ch;

        switch (ty = get_input(&ch)) {
        case INPUT_TYPE_NORMAL: {
                if (ch == '\t') {
                        assert(0);
                } else if (BACKSPACE(ch)) {
                        return backspace(b);
                } else {
                        return insert_char(b, ch, 1);
                }
        } break;
        case INPUT_TYPE_CTRL: {
                if (ch == CTRL_N) {
                        return down(b);
                } else if (ch == CTRL_P) {
                        return up(b);
                } else if (ch == CTRL_F) {
                        return right(b);
                } else if (ch == CTRL_B) {
                        return left(b);
                } else if (ch == CTRL_E) {
                        return eol(b);
                } else if (ch == CTRL_A) {
                        return bol(b);
                } else if (ch == CTRL_K) {
                        return kill_line(b);
                } else if (ch == CTRL_O) {
                        insert_char(b, '\n', 0);
                        --b->cx;
                        return BA_REDRAW;
                } else if (ch == CTRL_Q) {
                        exit(0);
                } else if (ch == CTRL_H) {
                        return backspace(b);
                }
        } break;
        case INPUT_TYPE_ALT: {
                if (ch == 'f')
                        return jump_next_word(b);
                else if (ch == 'b')
                        return jump_prev_word(b);
                else if (ch == 'd')
                        return del_word(b);
                else if (ch == '}')
                        return next_paragraph(b);
                else if (ch == '{')
                        return prev_paragraph(b);
        } break;
        default: break;
        }

        return BA_NOP;
}

static void
drawln(const buffer *b, size_t idx)
{
        if (idx < b->voff || idx >= b->voff + get_win_hight(b))
                return;

        const line *ln = b->lines.data[idx];
        const str *s = &ln->txt;
        unsigned y = b->size.hs + (unsigned)(idx - b->voff);
        unsigned win_w = get_win_width(b);
        unsigned tabw = TAB_WIDTH;

        // Clear line
        gotoxy(b->size.ws, y);
        for (unsigned x = 0; x < win_w; ++x)
                putchar(' ');

        if (b->hoff >= visual_column(s, s->len, tabw))
                return;

        // Draw text with horizontal scroll + tab expansion
        gotoxy(b->size.ws, y);

        unsigned screen_col = 0;
        unsigned char_i = 0;

        // Skip characters until we reach the horizontal scroll offset (visual)
        while (char_i < s->len && visual_column(s, char_i, tabw) < b->hoff) {
                ++char_i;
        }

        // draw visible part
        while (char_i < s->len && screen_col < win_w) {
                char c = s->chars[char_i];

                if (c == '\t') {
                        unsigned next_stop = (unsigned)(tabw - ((b->hoff + screen_col) % tabw));
                        for (unsigned t = 0; t < next_stop && screen_col < win_w; ++t) {
                                putchar(' ');
                                ++screen_col;
                        }
                } else {
                        putchar(c);
                        ++screen_col;
                }
                ++char_i;
        }
}

void
buffer_drawxy(const buffer *b)
{
        drawln(b, b->cy);

        const str *s = &b->lines.data[b->al]->txt;
        unsigned visual_x = visual_column(s, b->cx, TAB_WIDTH);

        unsigned screen_x = b->size.ws + (unsigned)(visual_x > b->hoff ? visual_x - b->hoff : 0);
        unsigned screen_y = b->size.hs + (unsigned)(b->cy - b->voff);

        gotoxy(screen_x, screen_y);
}

void
buffer_draw(const buffer *b)
{
        unsigned win_w = get_win_width(b);
        unsigned win_h = get_win_hight(b);

        // Clear the entire buffer area
        for (unsigned y = 0; y < win_h; ++y) {
                gotoxy(b->size.ws, b->size.hs + y);
                for (unsigned x = 0; x < win_w; ++x)
                        putchar(' ');
        }

        // Draw all visible lines once
        for (size_t i = 0; i < win_h; ++i) {
                size_t idx = b->voff + i;
                if (idx >= b->lines.len)
                        break;
                drawln(b, idx);
        }

        // Place cursor
        const str *s = &b->lines.data[b->al]->txt;
        unsigned visual_x = visual_column(s, b->cx, TAB_WIDTH);
        unsigned screen_x = b->size.ws + (unsigned)(visual_x > b->hoff ? visual_x - b->hoff : 0);
        unsigned screen_y = b->size.hs + (unsigned)(b->cy - b->voff);
        gotoxy(screen_x, screen_y);
}
