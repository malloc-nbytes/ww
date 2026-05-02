#include "buffer.h"
#include "mem.h"
#include "term.h"
#include "colors.h"
#include "glconf.h"
#include "config.h"
#include "minibuffer.h"
#include "io.h"
#include "pair.h"
#include "helpbuf.h"
#include "flags.h"

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

PAIR_DEFINE(int, int, int_pair);
PAIR_IMPL(int, int, int_pair);

ARRAY_DEFINE(int_pair, int_pair_ar);

static int
line_selection_range(const buffer *b,
                     size_t        idx,
                     size_t        line_len,
                     size_t       *sel_start,
                     size_t       *sel_end);

static void
draw_status(const buffer *b,
            const char   *msg);

static buffer_action
center_view(buffer *b);

char_ar g_cpy_buf = {0};

void
buffer_jump_to_verts(buffer *b,
                     size_t  x,
                     size_t  y)
{
        if (y > b->lines.len-1)
                return;
        if (x > b->lines.data[y]->txt.len-1)
                return;
        b->cx       = (unsigned)x;
        b->cy       = (unsigned)y;
        b->al       = (unsigned)y;
        b->wish_col = (unsigned)x;
        buffer_adjust_scroll(b);
}

void
buffer_free(buffer *b)
{
        str_destroy(&b->name);
        str_destroy(&b->path);
        str_destroy(&b->last_search);

        free(b);
}

void
buffer_make_builtin(buffer *b)
{
        b->builtin = 1;
}

buffer *
ww_helpbuf_alloc(unsigned  w,
                 unsigned  h,
                 unsigned  ws,
                 unsigned  hs,
                 ww       *parent)
{
        buffer *b;

        b = buffer_from(str_from(BUFFER_BUILTIN_HELP),
                        str_from(BUFFER_BUILTIN_HELP),
                        w, h, ws, hs, lines_from(HELP_DEF CONTROLS_DEF),
                        parent);

        buffer_make_readonly(b);
        buffer_make_builtin(b);

        return b;
}

buffer *
buffer_from(str      name,
            str      path,
            unsigned w,
            unsigned h,
            unsigned ws,
            unsigned hs,
            linep_ar lns,
            ww       *parent)
{
        buffer *b;

        b              = (buffer *)alloc(sizeof(buffer));
        b->name        = name;
        b->path        = path;
        b->builtin     = 0;
        b->size.w      = w;
        b->size.h      = h;
        b->size.ws     = ws;
        b->size.hs     = hs;
        b->lines       = lns;
        b->cx          = 0;
        b->cy          = 0;
        b->wish_col    = 0;
        b->al          = 0;
        b->hoff        = 0;
        b->voff        = 0;
        b->state       = BS_NORMAL;
        b->saved       = 1;
        b->sx          = 0;
        b->sy          = 0;
        b->writable    = 1;
        b->last_search = str_create();
        b->parent      = parent;
        b->last_tab    = 0;

        return b;
}

void
buffer_make_readonly(buffer *b)
{
        b->writable = 0;
}


static int_ar
find_line_matches(const buffer *b,
                  const str    *s)
{
        assert(b->state == BS_SEARCH);

        static int_ar     ar;
        const char       *sraw;
        const char       *query;

        ar    = array_empty(int_ar);
        sraw  = str_cstr(s);
        query = str_cstr(&b->last_search);

        if (!sraw || !query || !str_len(&b->last_search))
                return ar;

        for (size_t i = 0; i < str_len(s); ++i) {
                if (!memcmp(query, sraw+i, str_len(&b->last_search))) {
                        array_append(ar, (int)i);
                        i += str_len(&b->last_search)-1;
                }
        }

        return ar;
}

static int_pair_ar
find_all_matches_in_buffer(buffer *b)
{
        int_pair_ar pairs = array_empty(int_pair_ar);
        for (size_t i = 0; i < b->lines.len; ++i) {
                int_ar verts = find_line_matches(b, &b->lines.data[i]->txt);
                for (size_t j = 0; j < verts.len; ++j)
                        array_append(pairs, int_pair_create((int)i, verts.data[j]));
        }
        return pairs;
}

static void
search(buffer *b, int reverse)
{
        input_type  ty;
        char        ch;
        str        *input;
        int         first;
        unsigned    old_cx;
        unsigned    old_cy;
        size_t      old_al;
        int         step;
        int         adjust;

        input       = &b->last_search;
        b->state    = BS_SEARCH;
        first       = 1;
        old_cx      = b->cx;
        old_cy      = b->cy;
        old_al      = b->al;
        step        = 0;
        adjust      = 1;

        while (1) {
                int_pair_ar pairs = find_all_matches_in_buffer(b);

                if (adjust) {
                        step = 0;

                        for (size_t i = 0; i < pairs.len; ++i) {
                                if (pairs.data[i].l < (int)b->al)
                                        ++step;
                                else
                                        break;
                        }

                        if (reverse && step > 0) {
                                reverse = 0;
                                --step;
                        }
                }

                adjust = 0;

                if (pairs.len > 0 && step < (int)pairs.len) {
                        b->al = (size_t)pairs.data[step].l;
                        b->cy = (unsigned)pairs.data[step].l;
                        b->cx = (unsigned)pairs.data[step].r;
                        buffer_adjust_scroll(b);
                }

                center_view(b);
                buffer_draw(b);

                gotoxy(0, b->size.h);
                clear_line(0, b->size.h);
                printf("Search [ %s", str_cstr(input));
                fflush(stdout);

                ty = get_input(&ch);
                if (ty == INPUT_TYPE_NORMAL) {
                        if (first && (BACKSPACE(ch)
                                      || (ty == INPUT_TYPE_NORMAL && ch != '\n'))) {
                                b->cx = old_cx; b->cy = old_cy; b->al = old_al;
                                str_clear(&b->last_search);
                                first  = 0;
                                adjust = 1;
                        }
                        if (BACKSPACE(ch)) {
                                b->cx = old_cx; b->cy = old_cy; b->al = old_al;
                                if (str_len(input) > 0)
                                        str_pop(input);
                                adjust = 1;
                        } else if (ENTER(ch)) {
                                if (pairs.len > 0 && step < (int)pairs.len) {
                                        b->al       = (size_t)pairs.data[step].l;
                                        b->cy       = (unsigned)pairs.data[step].l;
                                        b->cx       = (unsigned)pairs.data[step].r;
                                        b->wish_col = b->cx;
                                }
                                break;
                        } else {
                                b->cx = old_cx; b->cy = old_cy; b->al = old_al;
                                adjust = 1;
                                str_append(input, ch);
                        }
                } else if (ty == INPUT_TYPE_CTRL && ch == CTRL_S) {
                        if (step < (int)pairs.len-1)
                                ++step;
                } else if (ty == INPUT_TYPE_CTRL && ch == CTRL_R) {
                        if (step > 0)
                                --step;
                } else if (ty == INPUT_TYPE_CTRL && ch == CTRL_Y && g_cpy_buf.len > 0) {
                        if (first) {
                                b->cx = old_cx; b->cy = old_cy; b->al = old_al;
                                str_clear(&b->last_search);
                                first  = 0;
                                adjust = 1;
                        }
                        for (size_t i = 0; i < g_cpy_buf.len; ++i)
                                str_append(input, g_cpy_buf.data[i]);
                } else {
                        b->cx = old_cx; b->cy = old_cy; b->al = old_al;
                        break;
                }
        }

        b->state = BS_NORMAL;

        center_view(b);
        buffer_adjust_scroll(b);
}

static int
writable(const buffer *b)
{
        if (!b->writable) {
                draw_status(b, "buffer is read-only");
                fflush(stdout);
                return 0;
        }

        return 1;
}

static void
clear_cpy(void)
{
        array_clear(g_cpy_buf);
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
visual_width_up_to(const str *s,
                   size_t     char_idx,
                   unsigned   tab_width)
{
        return visual_column(s, char_idx, tab_width);
}

static size_t
char_index_at_visual_col(const str *s,
                         unsigned   target_col,
                         unsigned   tab_width)
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
        unsigned   x = visual_column(s, b->cx, TAB_WIDTH);
        gotoxy(b->size.ws + (unsigned)(x > b->hoff ? x - b->hoff : 0U),
               b->size.hs + (unsigned)(b->cy - b->voff));
}

static unsigned
get_win_hight(const buffer *b)
{
        return b->size.h > 0 ? b->size.h - 1 - b->size.hs : 0;
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

buffer_action
buffer_adjust_scroll(buffer *b)
{
        int res;

        res = adjust_vscroll(b);
        res = adjust_hscroll(b) || res;

        return res ? BA_REDRAW : BA_NOP;
}

static buffer_action
del_selection(buffer *b)
{
        if (b->state != BS_SELECTION)
                return BA_NOP;

        size_t anchor_y = (size_t)b->sy;
        size_t anchor_x = (size_t)b->sx;
        size_t cursor_y = b->al;
        size_t cursor_x = b->cx;

        // normalize
        int forward = (anchor_y < cursor_y) ||
                (anchor_y == cursor_y && anchor_x <= cursor_x);

        size_t start_y = forward ? anchor_y : cursor_y;
        size_t end_y   = forward ? cursor_y : anchor_y;
        size_t start_x = forward ? anchor_x : cursor_x;
        size_t end_x   = forward ? cursor_x : anchor_x;

        if (start_y >= b->lines.len || end_y >= b->lines.len)
                goto cleanup;

        b->saved = 0;

        if (start_y == end_y) {
                // single-line deletion
                line *ln = b->lines.data[start_y];

                size_t remove_count = end_x - start_x;
                for (size_t i = 0; i < remove_count; ++i)
                        str_rm(&ln->txt, start_x);

                b->cx = (unsigned)start_x;
                b->al = (unsigned)start_y;
                b->cy = (unsigned)start_y;
        } else {
                // multi-line deletion

                // first line
                {
                        line *first = b->lines.data[start_y];
                        size_t len_first = str_len(&first->txt);
                        if (start_x < len_first) {
                                while (str_len(&first->txt) > start_x)
                                        str_rm(&first->txt, start_x);
                        }
                }

                // last line
                {
                        line *last = b->lines.data[end_y];
                        for (size_t i = 0; i < end_x; ++i)
                                str_rm(&last->txt, 0);
                }

                // delete all middle lines
                size_t lines_to_remove = end_y - start_y - 1;
                for (size_t i = 0; i < lines_to_remove; ++i) {
                        line_free(b->lines.data[start_y + 1]);
                        array_rm_at(b->lines, start_y + 1);
                }

                // join the first and last lines
                line *first = b->lines.data[start_y];
                line *last  = b->lines.data[start_y + 1];

                str_concat(&first->txt, str_cstr(&last->txt));
                line_free(last);
                array_rm_at(b->lines, start_y + 1);

                // place cursor at the join point
                b->cx = (unsigned)start_x;
                b->al = (unsigned)start_y;
                b->cy = (unsigned)start_y;
        }

 cleanup:
        b->wish_col = b->cx;
        b->state    = BS_NORMAL;

        buffer_adjust_scroll(b);
        return BA_REDRAW;
}

static buffer_action
up(buffer *b)
{
        if (b->cy > 0) {
                const str *olds = &b->lines.data[b->al]->txt;
                unsigned desired = visual_column(olds, /*b->wish_col*/b->cx, TAB_WIDTH);
                --b->cy;
                --b->al;
                const str *news = &b->lines.data[b->al]->txt;
                b->cx = (unsigned)char_index_at_visual_col(news, desired, TAB_WIDTH);
                if (desired < b->wish_col)
                        b->cx = b->wish_col;
        }

        if (b->cx > b->lines.data[b->al]->txt.len-1)
                b->cx = (unsigned)b->lines.data[b->al]->txt.len-1;

        adjust_cursor(b);
        return buffer_adjust_scroll(b) == BA_REDRAW || b->state == BS_SELECTION ? BA_REDRAW : BA_XY;
}

static buffer_action
down(buffer *b)
{
        if (b->cy < b->lines.len-1) {
                const str *olds = &b->lines.data[b->al]->txt;
                unsigned desired = visual_column(olds, /*b->wish_col*/b->cx, TAB_WIDTH);
                ++b->cy;
                ++b->al;
                const str *news = &b->lines.data[b->al]->txt;
                b->cx = (unsigned)char_index_at_visual_col(news, desired, TAB_WIDTH);
                if (desired < b->wish_col)
                        b->cx = b->wish_col;

        }

        if (b->cx > b->lines.data[b->al]->txt.len-1)
                b->cx = (unsigned)b->lines.data[b->al]->txt.len-1;

        adjust_cursor(b);
        return buffer_adjust_scroll(b) == BA_REDRAW || b->state == BS_SELECTION ? BA_REDRAW : BA_XY;
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
        b->wish_col = b->cx;

        adjust_cursor(b);
        return buffer_adjust_scroll(b) == BA_REDRAW || b->state == BS_SELECTION ? BA_REDRAW : BA_XY;
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
        b->wish_col = b->cx;

        adjust_cursor(b);
        return buffer_adjust_scroll(b) == BA_REDRAW || b->state == BS_SELECTION ? BA_REDRAW : BA_XY;
}

static buffer_action
bol(buffer *b)
{
        b->cx = 0;
        b->wish_col = 0;
        adjust_cursor(b);
        return buffer_adjust_scroll(b) == BA_REDRAW || b->state == BS_SELECTION ? BA_REDRAW : BA_XY;
}

static buffer_action
eol(buffer *b)
{
        str *s = &b->lines.data[b->al]->txt;
        b->cx = (unsigned)str_len(s)-1;
        b->wish_col = (unsigned)str_len(s)-1;
        adjust_cursor(b);
        return buffer_adjust_scroll(b) == BA_REDRAW || b->state == BS_SELECTION ? BA_REDRAW : BA_XY;
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
        return buffer_adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
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
        return buffer_adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
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

        buffer_adjust_scroll(b);
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
        return buffer_adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
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
        return buffer_adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
kill_line(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

        line *ln;
        const str *s;

        if (b->lines.len <= 0)
                return BA_NOP;

        ln = b->lines.data[b->al];
        s  = &ln->txt;

        clear_cpy();
        for (size_t i = 0; i < str_len(s); ++i)
                array_append(g_cpy_buf, str_at(s, i));

        line_free(ln);
        array_rm_at(b->lines, b->al);

        if (b->al > b->lines.len-1) {
                --b->al;
                --b->cy;
        }

        b->cx       = 0;
        b->wish_col = 0;

        buffer_adjust_scroll(b);
        return BA_REDRAW;
}

static buffer_action
insert_char(buffer *b, char ch, int newline_advance)
{
        if (!writable(b))
                return BA_NOP;

        b->saved = 0;

        if (!b->lines.data) {
                char tmp[] = {'\n', 0};
                array_append(b->lines, line_from(str_from(tmp)));
                /* array_append(b->lines, line_from(str_from("\n"))); */
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
                        b->wish_col = 0;
                        ++b->cy;
                        ++b->al;
                }
        }

        //add_to_popxy(b);

        adjust_cursor(b);
        return (buffer_adjust_scroll(b) == BA_REDRAW || ch == '\n') ? BA_REDRAW : BA_XY;
}

static buffer_action
jump_to_top_of_buffer(buffer *b)
{
        if (b->lines.len == 0) // not sure if this is required, but doesn't hurt
                return BA_NOP;

        b->cy = (unsigned)b->lines.len-1;
        b->cx = 0;
        b->wish_col = 0;
        b->al = b->lines.len-1;
        adjust_cursor(b);
        return buffer_adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
jump_to_bottom_of_buffer(buffer *b)
{
        b->cy = 0;
        b->cx = 0;
        b->wish_col = 0;
        b->al = 0;
        adjust_cursor(b);
        return buffer_adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
del_char(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

        if (b->state == BS_SELECTION)
                return del_selection(b);

        line *ln;
        int   newline;

        ln       = b->lines.data[b->al];
        newline  = 0;
        b->saved = 0;

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
        return (buffer_adjust_scroll(b) == BA_REDRAW || newline) ? BA_REDRAW : BA_XY;
}

static buffer_action
backspace(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

        line *ln;
        int   newline;

        ln       = b->lines.data[b->al];
        newline  = 0;
        b->saved = 0;

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

                buffer_adjust_scroll(b);
                return BA_REDRAW;
        }

        if (((glconf.flags & FK_TABMODE) == 0) && b->last_tab > 0) {
                --b->last_tab;
                for (size_t i = 0; i < (size_t)glconf.runtime.space_amt; ++i) {
                        left(b);
                        str_rm(&ln->txt, b->cx);
                        if (b->cx > str_len(&ln->txt)-1)
                                b->cx = (unsigned)str_len(&ln->txt)-1;
                }
        } else {
                left(b);
                str_rm(&ln->txt, b->cx);
                if (b->cx > str_len(&ln->txt)-1)
                        b->cx = (unsigned)str_len(&ln->txt)-1;
        }

        //add_to_popxy(b);
        return (buffer_adjust_scroll(b) == BA_REDRAW || newline) ? BA_REDRAW : BA_XY;
}

static buffer_action
delete_until_eol(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

        line *ln;
        const str *s;

        ln = b->lines.data[b->al];
        s  = &ln->txt;

        clear_cpy();
        for (size_t i = b->cx; i < str_len(s)-1; ++i)
                array_append(g_cpy_buf, str_at(s, i));

        str_cut(&ln->txt, b->cx);
        str_insert(&ln->txt, b->cx, '\n');

        //add_to_popxy(b);

        buffer_adjust_scroll(b);

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

        b->wish_col = b->cx;

        adjust_cursor(b);
        return buffer_adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
center_view(buffer *b)
{
        int rows = (int)get_win_hight(b);
        int vertical_offset = (int)b->cy - (rows/2);
        if (vertical_offset < 0)
                vertical_offset = 0;

        int max_offset = (int)b->lines.len - rows;
        if (max_offset < 0)
                max_offset = 0;

        b->voff = (unsigned)vertical_offset;
        buffer_adjust_scroll(b);

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

        l0  = b->lines.data[b->al];
        s0  = &l0->txt;
        l1  = b->lines.data[b->al+1];
        s1  = &l1->txt;
        len = str_len(s0);

        s0->chars[s0->len-1] = ' ';
        str_trim_before(s1);
        str_concat(s0, str_cstr(s1));
        array_rm_at(b->lines, b->al+1);

        b->cx = (unsigned)len-1;
        b->wish_col = b->cx;

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
        b->wish_col = 0;

        buffer_adjust_scroll(b);

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
        b->wish_col = 0;

        buffer_adjust_scroll(b);

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
        if (!writable(b))
                return BA_NOP;

        line   *ln;
        size_t  start;

        ln    = b->lines.data[b->al];
        start = b->cx;

        // cursor at beginning of line fall back to regular backspace
        if (b->cx == 0)
                return backspace(b);

        b->saved = 0;

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
        b->last_tab = 0;

        //add_to_popxy(b);
        return buffer_adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
buffer_dupline(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

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
        buffer_adjust_scroll(b);
        return BA_REDRAW;
}

static buffer_action
movetxt_up(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

        if (b->al <= 0)
                return BA_NOP;

        line *tmp;

        tmp                    = b->lines.data[b->al];
        b->lines.data[b->al]   = b->lines.data[b->al-1];
        b->lines.data[b->al-1] = tmp;

        --b->al;
        --b->cy;
        //add_to_popxy(b);
        buffer_adjust_scroll(b);
        return BA_REDRAW;
}

static buffer_action
movetxt_down(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

        if (b->al >= b->lines.len-1)
                return BA_NOP;

        line *tmp;

        tmp                    = b->lines.data[b->al];
        b->lines.data[b->al]   = b->lines.data[b->al+1];
        b->lines.data[b->al+1] = tmp;

        ++b->al;
        ++b->cy;

        //add_to_popxy(b);
        buffer_adjust_scroll(b);
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
        b->wish_col = b->cx;

        //add_to_popxy(b);

        return BA_XY;
}

static buffer_action
uppercase_word(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

        upperlower_word(b, toupper, 0);

        //add_to_popxy(b);
        buffer_adjust_scroll(b);
        return buffer_adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
lowercase_word(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

        upperlower_word(b, tolower, 1);

        //add_to_popxy(b);
        buffer_adjust_scroll(b);
        return buffer_adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
caps_word(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

        upperlower_word(b, toupper, 1);

        //add_to_popxy(b);
        return buffer_adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
swap_chars(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

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
        b->wish_col = b->cx;

        //add_to_popxy(b);
        return BA_XY;
}

static buffer_action
cancel(buffer *b)
{
        b->state = BS_NORMAL;
        return BA_REDRAW;
}

static buffer_action
selection(buffer *b)
{
        if (b->state == BS_NORMAL)
                b->state = BS_SELECTION;
        else if (b->state == BS_SELECTION)
                b->state = BS_NORMAL;
        else
                return BA_NOP;

        b->sy = (unsigned)b->al;
        b->sx = b->cx;

        return BA_XY;
}

static buffer_action
copy_selection(buffer *b)
{
        if (b->state != BS_SELECTION)
                return BA_NOP;

        clear_cpy();

        size_t start_line, start_col, end_line, end_col;

        (void)end_col;
        (void)start_col;

        // determine absolute selection bounds
        if (b->sy < b->al || (b->sy == b->al && b->sx <= b->cx)) {
                start_line = b->sy;
                start_col  = b->sx;
                end_line   = b->al;
                end_col    = b->cx;
        } else {
                start_line = b->al;
                start_col  = b->cx;
                end_line   = b->sy;
                end_col    = b->sx;
        }

        // copy lines
        for (size_t i = start_line; i <= end_line; ++i) {
                str *ln = &b->lines.data[i]->txt;
                size_t sel_start, sel_end;

                if (!line_selection_range(b, i, ln->len, &sel_start, &sel_end))
                        continue;

                for (size_t j = sel_start; j < sel_end; ++j)
                        array_append(g_cpy_buf, ln->chars[j]);
        }

        b->state = BS_NORMAL;

        return BA_REDRAW;
}

static buffer_action
cut_selection(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

        if (b->state != BS_SELECTION)
                return BA_NOP;

        copy_selection(b);
        b->state = BS_SELECTION;
        buffer_action a = del_selection(b);
        //add_to_popxy(b);
        return a == BA_REDRAW
                ? BA_REDRAW : BA_XY;
}

static buffer_action
paste(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

        int newline;

        newline = 0;

        if (b->state == BS_SELECTION) {
                del_selection(b);
                newline = 1;
        }

        for (size_t i = 0; i < g_cpy_buf.len; ++i) {
                insert_char(b, g_cpy_buf.data[i], 1);
                if (g_cpy_buf.data[i] == '\n')
                        newline = 1;
        }

        //add_to_popxy(b);

        return (buffer_adjust_scroll(b) == BA_REDRAW || newline)
                ? BA_REDRAW : BA_XY;
}

buffer_action
buffer_save(buffer *b)
{
        if (!writable(b))
                return BA_NOP;

        char_ar content = array_empty(char_ar);
        for (size_t i = 0; i < b->lines.len; ++i) {
                const line *ln = b->lines.data[i];
                for (size_t j = 0; j < ln->txt.len; ++j) {
                        array_append(content, ln->txt.chars[j]);
                }
        }
        array_append(content, 0);
        if (!write_file(b->path.chars, content.data)) {
                perror("write_file");
                return BA_NOP;
        }

        b->saved = 1;
        return BA_XY;
}

static buffer_action
ctrlx(buffer *b)
{
        char       ch;
        input_type ty;

        switch (ty = get_input(&ch)) {
        case INPUT_TYPE_NORMAL: {
                if (ch == 'b')
                        return BA_REQ_SWITCHBUFFER;
                if (ch == '/')
                        return BA_REQ_SPLITVER;
                if (ch == 'o')
                        return BA_REQ_JMPBUF;
                if (ch == 'm')
                        return BA_REQ_MAXIMIZEMON;
                if (ch == 'x')
                        return BA_REQ_COMPILE;
                if (ch == '-')
                        return BA_REQ_SPLITHOR;
                if (ch == 'k')
                        return BA_REQ_KILLBUF;
        } break;
        case INPUT_TYPE_CTRL: {
                if (ch == CTRL_S)
                        return buffer_save(b);
                if (ch == CTRL_Q)
                        return BA_REQ_EXIT;
                if (ch == CTRL_F)
                        return BA_REQ_FINDFILE;
        } break;
        default: break;
        }

        return BA_NOP;
}

static buffer_action
handle_normal_input_while_builtin(buffer *b, char ch)
{
        if (!strcmp(b->name.chars, BUFFER_BUILTIN_COMPILE) && ch == 'g')
                return BA_REQ_RECOMPILE;
        if (!strcmp(b->name.chars, BUFFER_BUILTIN_COMPILE) && ch == '\n')
                return BA_REQ_ERRJMP;

        if (ch == 'q')
                return BA_REQ_CLOSE_BUILTIN;

        return BA_NONE;
}

static buffer_action
tab(buffer *b)
{
        buffer_action ba;

        ba = BA_NOP;

        ++b->last_tab;

        if (glconf.flags & FK_TABMODE)
                return insert_char(b, '\t', 1);

        for (size_t i = 0; i < (size_t)glconf.runtime.space_amt; ++i) {
                if (ba == BA_REDRAW)
                        insert_char(b, ' ', 1);
                else
                        ba = insert_char(b, ' ', 1);
        }

        return ba == BA_REDRAW ? BA_REDRAW : BA_XY;
}

static buffer_action
jmp_and_highlight_forward(buffer *b)
{
        if (b->state != BS_SELECTION)
                selection(b);
        jump_next_word(b);
        return buffer_adjust_scroll(b) == BA_REDRAW ? BA_REDRAW : BA_XY;
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
                if (!BACKSPACE(ch))
                        b->last_tab = 0;

                if (b->builtin) {
                        buffer_action ba;

                        if ((ba = handle_normal_input_while_builtin(b, ch)) != BA_NONE) {
                                return ba;
                        }
                }

                if (ch == '\t')         assert(0);
                else if (BACKSPACE(ch)) return backspace(b);
                else if (ch == 0)       return selection(b);
                else                    return insert_char(b, ch, 1);
        } break;
        case INPUT_TYPE_CTRL: {
                if (ch != 9)
                        b->last_tab = 0;

                if (ch == CTRL_N)      return down(b);
                else if (ch == 9)      return tab(b);
                else if (ch == CTRL_P) return up(b);
                else if (ch == CTRL_F) return right(b);
                else if (ch == CTRL_B) return left(b);
                else if (ch == CTRL_E) return eol(b);
                else if (ch == CTRL_A) return bol(b);
                else if (ch == CTRL_K) return delete_until_eol(b);
                else if (ch == CTRL_O) {
                        if (insert_char(b, '\n', 0) != BA_NOP) {
                                --b->cx;
                                return BA_REDRAW;
                        }
                        return BA_NOP;
                }
                else if (ch == CTRL_H) return backspace(b);
                else if (ch == CTRL_D) return del_char(b);
                else if (ch == CTRL_L) return center_view(b);
                else if (ch == CTRL_V) return page_down(b);
                else if (ch == CTRL_T) return swap_chars(b);
                else if (ch == CTRL_G) return cancel(b);
                else if (ch == CTRL_Y) return paste(b);
                else if (ch == CTRL_W) return cut_selection(b);
                else if (ch == CTRL_X) return ctrlx(b);
                else if (ch == CTRL_S || ch == CTRL_R) {
                        search(b, ch == CTRL_R);
                        return BA_REDRAW;
                }
        } break;
        case INPUT_TYPE_ALT: {
                b->last_tab = 0;
                if (ch == 'f')          return jump_next_word(b);
                else if (ch == 'b')     return jump_prev_word(b);
                else if (ch == 'd')     return del_word(b);
                else if (ch == '}')     return next_paragraph(b);
                else if (ch == '{')     return prev_paragraph(b);
                else if (ch == '>')     return jump_to_top_of_buffer(b);
                else if (ch == '<')     return jump_to_bottom_of_buffer(b);
                else if (ch == 'k')     return kill_line(b);
                else if (ch == 'm')     return jump_to_first_char(b);
                else if (ch == 'j')     return combine_lines(b);
                else if (ch == 'v')     return page_up(b);
                else if (BACKSPACE(ch)) return super_backspace(b);
                else if (ch == '\\')    return buffer_dupline(b);
                else if (ch == 'n')     return movetxt_down(b);
                else if (ch == 'p')     return movetxt_up(b);
                else if (ch == 'u')     return caps_word(b);
                else if (ch == 'l')     return lowercase_word(b);
                else if (ch == 'c')     return uppercase_word(b);
                else if (ch == 'w')     return copy_selection(b);
                else if (ch == 'x')     return BA_REQ_METAX;
                else if (ch == '.')     return jmp_and_highlight_forward(b);
                else if (ch == '\t')    return BA_REQ_SWITCHCOMPL;
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

        gotoxy(0, (unsigned)glconf.term.h);

        printf(INVERT);

        sprintf(buf, "[ww-v" VERSION "] %s:%d:%d%s %s Monitor:%d",
                str_cstr(&b->name),
                b->cy+1,
                b->cx+1,
                !b->saved ? "*" : "",
                state_to_cstr(b),
                b->parent->am);
        printf("%s", buf);
        len += strlen(buf);

        if (msg) {
                sprintf(buf, " [%s" RESET INVERT "]", msg);
                printf("%s", buf);
                len += strlen(buf);
        }

        for (size_t i = len; i < glconf.term.w; ++i)
                putchar(' ');

        printf(RESET);

        gotoxy(b->cx - (unsigned)b->hoff, b->cy - (unsigned)b->voff);
}

static int
line_selection_range(const buffer *b,
                     size_t        idx,
                     size_t        line_len,
                     size_t       *sel_start,
                     size_t       *sel_end)
{
        // Computes the selection range for a given line
        // Returns 1 if there is a selection on this line, 0 otherwise

        if (b->state != BS_SELECTION)
                return 0;

        size_t start_line, start_col, end_line, end_col;

        // absolute selection bounds
        if (b->sy < b->al || (b->sy == b->al && b->sx <= b->cx)) {
                start_line = b->sy;
                start_col  = b->sx;
                end_line   = b->al;
                end_col    = b->cx;
        } else {
                start_line = b->al;
                start_col  = b->cx;
                end_line   = b->sy;
                end_col    = b->sx;
        }

        if (idx < start_line || idx > end_line)
                return 0; // no selection on this line

        if (start_line == end_line) {
                *sel_start = start_col;
                *sel_end   = end_col;
        } else if (idx == start_line) {
                *sel_start = start_col;
                *sel_end   = line_len;
        } else if (idx == end_line) {
                *sel_start = 0;
                *sel_end   = end_col;
        } else {
                *sel_start = 0;
                *sel_end   = line_len;
        }

        return 1;
}

static ssize_t
find_trailing_whitespace_start(const str *s)
{
        if (s->len == 0)
                return -1;
        if (s->len <= 1 || !isspace(s->chars[s->len-2]))
                return -1;
        for (int i = (int)s->len-1; i >= 0; --i) {
                if (!isspace(s->chars[i]))
                        return (ssize_t)i;
        }
        return 0;
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

        // clear line
        gotoxy(b->size.ws, y);
        for (unsigned x = 0; x < win_w; ++x)
                putchar(' ');

        if (b->hoff >= visual_column(s, s->len, tabw))
                return;

        gotoxy(b->size.ws, y);
        unsigned screen_col = 0;
        size_t char_i = 0;

        // skip characters until horizontal scroll offset
        while (char_i < s->len && visual_column(s, char_i, tabw) < b->hoff) {
                ++char_i;
        }

        // determine selection range on this line
        size_t sel_start = 0, sel_end = 0;
        int cursor_on_line = ((size_t)b->cy == idx);
        int line_has_selection = line_selection_range(b, idx, s->len, &sel_start, &sel_end);
        int_ar search_matches = {0};
        size_t qlen = str_len(&b->last_search);
        size_t match_idx = 0;

        if (b->state == BS_SEARCH)
                search_matches = find_line_matches(b, s);

        ssize_t whitespace_start = find_trailing_whitespace_start(s);

        // draw visible part
        while (char_i < s->len && screen_col < win_w) {
                char c = s->chars[char_i];
                int in_search = 0;
                int in_cursor_match = 0;

                if (b->state == BS_SEARCH && search_matches.len > 0) {
                        if (match_idx < search_matches.len) {
                                int mstart = search_matches.data[match_idx];
                                int mend = mstart + (int)qlen;
                                if ((int)char_i >= mstart && (int)char_i < mend) {
                                        in_search = 1;
                                        if (cursor_on_line && (int)b->cx >= mstart && (int)b->cx < mend) {
                                                in_cursor_match = 1;
                                        }
                                } else if ((int)char_i >= mend) {
                                        ++match_idx;
                                }
                        }
                }

                int in_selection = line_has_selection && char_i >= sel_start && char_i < sel_end;

                // Handle tab character
                if (c == '\t') {
                        unsigned next_stop = (unsigned)(tabw - ((b->hoff + screen_col) % tabw));
                        for (unsigned t = 0; t < next_stop && screen_col < win_w; ++t) {
                                if (in_selection) {
                                        printf(INVERT YELLOW BOLD " " RESET);
                                } else if (in_cursor_match) {
                                        printf(INVERT ORANGE BOLD " " RESET);
                                } else if (in_search) {
                                        printf(INVERT YELLOW BOLD " " RESET);
                                } else {
                                        if (/*b->show_trailing_whitespace && */t == 0)
                                                printf(GRAY ">" RESET);
                                        else
                                                putchar(' ');
                                }
                                ++screen_col;
                        }
                } else {
                        int is_trailing_whitespace =
                                (whitespace_start != -1)
                                && (c != '\n')
                                && (((size_t)whitespace_start < char_i)
                                    || ((size_t)whitespace_start == 0
                                    && (size_t)whitespace_start <= char_i));

                        if (/*b->show_trailing_whitespace && */is_trailing_whitespace && !in_selection) {
                                printf(GRAY "-" RESET);
                        } else {
                                if (in_selection) {
                                        printf(INVERT YELLOW BOLD "%c" RESET, c);
                                } else if (in_cursor_match) {
                                        printf(INVERT ORANGE BOLD "%c" RESET, c);
                                } else if (in_search) {
                                        printf(INVERT YELLOW BOLD "%c" RESET, c);
                                } else {
                                        putchar(c);
                                }
                        }
                        ++screen_col;
                }
                ++char_i;
        }

        // Clean up search matches
        if (search_matches.data)
                array_free(search_matches);
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
        for (unsigned y = 0; y < win_h+1; ++y) {
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
        const str *s      = &b->lines.data[b->al]->txt;
        unsigned visual_x = visual_column(s, b->cx, TAB_WIDTH);
        unsigned screen_x = b->size.ws + (unsigned)(visual_x > b->hoff ? visual_x - b->hoff : 0);
        unsigned screen_y = b->size.hs + (unsigned)(b->cy - b->voff);

        draw_status(b, NULL);
        gotoxy(screen_x, screen_y);
}

void
init_buffer_translation_unit(void)
{
        g_cpy_buf = array_empty(char_ar);
}
