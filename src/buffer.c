#include "buffer.h"
#include "mem.h"
#include "term.h"
#include "colors.h"
#include "glconf.h"

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef __linux__
#include <limits.h>
#else
#define PATH_MAX 4096
#endif

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
        b->state   = BS_NORMAL;
        b->saved   = 1;

        return b;
}

static const char *
state_to_cstr(const buffer *b)
{
        switch (b->state) {
        case BS_NORMAL:    return "normal";
        case BS_SELECTION: return "selection";
        case BS_SEARCH:    return "search";
        default:           return "unknown";
        }
        return "unknown";
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

        adjust_scroll(b);
        return BA_REDRAW;
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

        line_free(ln);
        array_rm_at(b->lines, b->al);

        if (b->al > b->lines.len-1) {
                --b->al;
                --b->cy;
        }

        b->cx = 0;

        adjust_scroll(b);
        return BA_REDRAW;
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
jump_to_top_of_buffer(buffer *b)
{
        if (b->lines.len == 0) // not sure if this is required, but doesn't hurt
                return BA_NOP;

        b->cy = (unsigned)b->lines.len-1;
        b->cx = 0;
        //b->wish_col = 0;
        b->al = b->lines.len-1;
        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
jump_to_bottom_of_buffer(buffer *b)
{
        b->cy = 0;
        b->cx = 0;
        //b->wish_col = 0;
        b->al = 0;
        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
del_char(buffer *b)
{
        //if (!writable(b))
        //        return 0;

        /*if (b->state == BS_SELECTION) {
                del_selection(b);
                return 1;
        }*/

        line *ln;
        int   newline;

        ln       = b->lines.data[b->al];
        newline  = 0;
        //b->saved = 0;

        if (ln->txt.chars[b->cx] == '\n') {
                newline = 1;
                if (b->al < b->lines.len-1) {
                        str *s = &ln->txt;
                        str_concat(s, str_cstr(&b->lines.data[b->al+1]->txt));
                        line_free(b->lines.data[b->al+1]);
                        array_rm_at(b->lines, b->al+1);
                } else {
                        return 0;
                }
        }

        str_rm(&ln->txt, b->cx);
        if (b->cx > str_len(&ln->txt)-1)
                b->cx = (unsigned)str_len(&ln->txt)-1;

        //add_to_popxy(b);
        return (adjust_scroll(b) == BA_REDRAW || newline) ? BA_REDRAW : BA_XY;
}

static buffer_action
backspace(buffer *b)
{
        //if (!writable(b))
        //        return 0;

        line *ln;
        int   newline;

        ln       = b->lines.data[b->al];
        newline  = 0;
        //b->saved = 0;

        if (b->cx == 0) {
                if (b->al == 0)
                        return 0;
                line   *prevln     = b->lines.data[b->al-1];
                size_t  prevln_len = str_len(&prevln->txt);

                str_rm(&prevln->txt, prevln_len-1);
                str_concat(&prevln->txt, str_cstr(&ln->txt));
                line_free(b->lines.data[b->al]);
                array_rm_at(b->lines, b->al);

                --b->al;
                b->cx = (unsigned)prevln_len-1;
                --b->cy;

                adjust_scroll(b);
                return 1;
        }

        /*if (b->last_tab > 0 && (glconf.flags & FT_TABMODE) == 0) {
                --b->last_tab;
                for (size_t i = 0; i < (size_t)glconf.defaults.space_amt; ++i) {
                        buffer_left(b);
                        str_rm(&ln->s, b->cx);
                        if (b->cx > str_len(&ln->s)-1)
                                b->cx = str_len(&ln->s)-1;
                }
        } else {
                buffer_left(b);
                str_rm(&ln->s, b->cx);
                if (b->cx > str_len(&ln->s)-1)
                        b->cx = str_len(&ln->s)-1;
        }*/

        // TODO: remove this
        {
                left(b);
                str_rm(&ln->txt, b->cx);
                if (b->cx > str_len(&ln->txt)-1)
                        b->cx = (unsigned)str_len(&ln->txt)-1;
        }

        //add_to_popxy(b);
        return (adjust_scroll(b) == BA_REDRAW || newline) ? BA_REDRAW : BA_XY;
}


static buffer_action
delete_until_eol(buffer *b)
{
        //if (!writable(b))
        //        return;

        line *ln;
        //const str *s;

        ln = b->lines.data[b->al];
        //s  = &ln->txt;

        /*clear_cpy();
        for (size_t i = b->cx; i < str_len(s)-1; ++i)
                array_append(g_cpy_buf, str_at(s, i));*/

        str_cut(&ln->txt, b->cx);
        str_insert(&ln->txt, b->cx, '\n');

        //add_to_popxy(b);

        adjust_scroll(b);

        return BA_XY;
}

static buffer_action
jump_to_first_char(buffer *b)
{
        const str  *s;
        const char *sraw;

        s    = &b->lines.data[b->al]->txt;
        sraw = str_cstr(s);

        for (size_t i = 0; i < str_len(s); ++i) {
                if (sraw[i] != ' ' && sraw[i] != '\n' && sraw[i] != '\t' && sraw[i] != '\r') {
                        b->cx = (unsigned)i;
                        break;
                }
        }

        //b->wish_col = b->cx;

        adjust_cursor(b);
        return adjust_scroll(b);
}

static buffer_action
center_view(buffer *b)
{
        int rows = (int)b->size.h;
        int vertical_offset = (int)b->cy - (rows/2);
        if (vertical_offset < 0)
                vertical_offset = 0;

        int max_offset = (int)b->lines.len - rows;
        if (max_offset < 0)
                max_offset = 0;

        b->voff = (unsigned)vertical_offset;
        adjust_scroll(b);

        return BA_REDRAW;
}

static buffer_action
combine_lines(buffer *b)
{
        line   *l0;
        line   *l1;
        str    *s0;
        str    *s1;
        size_t len;

        if (b->al >= b->lines.len-1)
                return BA_NOP;

        l0 = b->lines.data[b->al];
        s0 = &l0->txt;
        l1 = b->lines.data[b->al+1];
        s1 = &l1->txt;
        len = str_len(s0);

        s0->chars[s0->len-1] = ' ';
        str_trim_before(s1);
        str_concat(s0, str_cstr(s1));
        array_rm_at(b->lines, b->al+1);

        b->cx = (unsigned)len-1;
        //b->wish_col = b->cx;

        //add_to_popxy(b);

        return BA_REDRAW;
}


static buffer_action
page_down(buffer *b)
{
        size_t h;

        h = b->size.h;

        if (b->al + h > b->lines.len) {
                b->al = b->lines.len-1;
                b->cy = (unsigned)b->lines.len-1;
        } else {
                b->al += h;
                b->cy += (unsigned)h;
        }

        b->cx       = 0;
        //b->wish_col = 0;

        adjust_scroll(b);

        return BA_REDRAW;
}

static buffer_action
page_up(buffer *b)
{
        size_t h;

        h = b->size.h;

        if ((int)b->al - (int)h < 0) {
                b->al = 0;
                b->cy = 0;
        } else {
                b->al -= h;
                b->cy -= (unsigned)h;
        }

        b->cx       = 0;
        //b->wish_col = 0;

        adjust_scroll(b);

        return BA_REDRAW;
}


static int
backspace_stop(unsigned char ch)
{
        return isspace(ch) || !isalnum(ch);
}

static int
super_backspace(buffer *b)
{
        //if (!writable(b))
        //        return 0;

        line   *ln;
        size_t  start;

        ln    = b->lines.data[b->al];
        start = b->cx;

        // cursor at beginning of line fall back to regular backspace
        if (b->cx == 0)
                return backspace(b);

        //b->saved = 0;

        while (start > 0 && backspace_stop((unsigned char)str_at(&ln->txt, start - 1)))
                --start;

        if (start > 0) {
                while (start > 0 && !backspace_stop((unsigned char)str_at(&ln->txt, start - 1)))
                        --start;
        }

        // nothing to remove
        if (start == b->cx)
                return 0;

        str_remove_range(&ln->txt, start, b->cx - start);

        b->cx       = (unsigned)start;
        //b->last_tab = 0;

        //add_to_popxy(b);
        return adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
buffer_dupline(buffer *b)
{
        //if (!writable(b))
        //        return;

        line *ln;
        str  *s;
        line *newln;

        ln = b->lines.data[b->al];
        s  = &ln->txt;
        newln = line_from_cstr(str_cstr(s));

        array_insert_at(b->lines, b->al, newln);
        ++b->al;
        ++b->cy;

        //add_to_popxy(b);
        adjust_scroll(b);
        return BA_REDRAW;
}

static buffer_action
movetxt_up(buffer *b)
{
        //if (!writable(b))
        //        return;

        if (b->al <= 0)
                return BA_NOP;

        line *tmp;

        tmp                    = b->lines.data[b->al];
        b->lines.data[b->al]   = b->lines.data[b->al-1];
        b->lines.data[b->al-1] = tmp;

        --b->al;
        --b->cy;
        //add_to_popxy(b);
        adjust_scroll(b);
        return BA_REDRAW;
}

static buffer_action
movetxt_down(buffer *b)
{
        //if (!writable(b))
        //        return;

        if (b->al >= b->lines.len-1)
                return BA_NOP;

        line *tmp;

        tmp                    = b->lines.data[b->al];
        b->lines.data[b->al]   = b->lines.data[b->al+1];
        b->lines.data[b->al+1] = tmp;

        ++b->al;
        ++b->cy;

        //add_to_popxy(b);
        adjust_scroll(b);
        return BA_REDRAW;
}

static buffer_action
upperlower_word(buffer *b,
                int   (*fun)(int),
                int     all)
{
        size_t      start;
        line       *ln;
        str        *s;
        const char *sraw;

        start = b->cx;
        ln    = b->lines.data[b->al];
        s     = &ln->txt;
        sraw  = str_cstr(s);

        while (start < str_len(s) && !isalpha(sraw[start]))
                ++start;

        if (start >= str_len(s))
                return BA_NOP;

        for (size_t i = 0; start < str_len(s) && (isalnum(sraw[start]) || sraw[start] == '_'); ++i, ++start) {
                if ((!all && !i) || all)
                        s->chars[start] = (char)fun(s->chars[start]);
        }

        b->cx = (unsigned)start;
        //b->wish_col = b->cx;

        //add_to_popxy(b);

        return BA_XY;
}

static buffer_action
uppercase_word(buffer *b)
{
        //if (!writable(b))
        //        return;
        upperlower_word(b, toupper, 0);

        //add_to_popxy(b);
        adjust_scroll(b);
        return adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
lowercase_word(buffer *b)
{
        //if (!writable(b))
        //        return;
        upperlower_word(b, tolower, 1);

        //add_to_popxy(b);
        adjust_scroll(b);
        return adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
caps_word(buffer *b)
{
        //if (!writable(b))
        //        return;
        upperlower_word(b, toupper, 1);

        //add_to_popxy(b);
        return adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
swap_chars(buffer *b)
{
        //if (!writable(b))
        //        return;

        line *ln;
        str  *s;
        char  ch;

        ln = b->lines.data[b->al];
        s  = &ln->txt;
        ch = str_at(s, b->cx);

        if (b->cx >= str_len(s)-1 || b->cx == 0)
                return BA_NOP;

        s->chars[b->cx]   = s->chars[b->cx-1];
        s->chars[b->cx-1] = ch;

        ++b->cx;
        //b->wish_col = b->cx;

        //add_to_popxy(b);
        return BA_XY;
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
                if (ch == CTRL_N)
                        return down(b);
                else if (ch == CTRL_P)
                        return up(b);
                else if (ch == CTRL_F)
                        return right(b);
                else if (ch == CTRL_B)
                        return left(b);
                else if (ch == CTRL_E)
                        return eol(b);
                else if (ch == CTRL_A)
                        return bol(b);
                else if (ch == CTRL_K)
                        return delete_until_eol(b);
                else if (ch == CTRL_O) {
                        insert_char(b, '\n', 0);
                        --b->cx;
                        return BA_REDRAW;
                } else if (ch == CTRL_Q)
                        exit(0);
                else if (ch == CTRL_H)
                        return backspace(b);
                else if (ch == CTRL_D)
                        return del_char(b);
                else if (ch == CTRL_L)
                        return center_view(b);
                else if (ch == CTRL_V)
                        return page_down(b);
                else if (ch == CTRL_T)
                        return swap_chars(b);
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
                else if (ch == '>')
                        return jump_to_top_of_buffer(b);
                else if (ch == '<')
                        return jump_to_bottom_of_buffer(b);
                else if (ch == 'k')
                        return kill_line(b);
                else if (ch == 'm')
                        return jump_to_first_char(b);
                else if (ch == 'j')
                        return combine_lines(b);
                else if (ch == 'v')
                        return page_up(b);
                else if (BACKSPACE(ch))
                        return super_backspace(b);
                else if (ch == '\\')
                        return buffer_dupline(b);
                else if (ch == 'n')
                        return movetxt_down(b);
                else if (ch == 'p')
                        return movetxt_up(b);
                else if (ch == 'u')
                        return caps_word(b);
                else if (ch == 'l')
                        return lowercase_word(b);
                else if (ch == 'c')
                        return uppercase_word(b);
        } break;
        default: break;
        }

        return BA_NOP;
}


static void
draw_status(const buffer *b,
            const char   *msg)
{
        char   buf[PATH_MAX + 32];
        size_t len;

        len = 0;

        gotoxy(0, b->size.h);

        printf(INVERT);

        sprintf(buf, "[ww-v" VERSION "] %s:%d:%d%s %s",
                str_cstr(&b->name),
                b->cy+1,
                b->cx+1,
                !b->saved ? "*" : "",
                state_to_cstr(b));
        printf("%s", buf);
        len += strlen(buf);

        if (msg) {
                sprintf(buf, " [%s" RESET INVERT "]", msg);
                printf("%s", buf);
                len += strlen(buf);
        }

        for (size_t i = len; i < b->size.w; ++i)
                putchar(' ');

        printf(RESET);

        gotoxy(b->cx - (unsigned)b->hoff, b->cy - (unsigned)b->voff);
        //fflush(stdout);
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

        draw_status(b, NULL);
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

        draw_status(b, NULL);
        gotoxy(screen_x, screen_y);
}
