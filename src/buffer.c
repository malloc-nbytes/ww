#include "buffer.h"
#include "io.h"
#include "term.h"
#include "utils.h"
#include "window.h"
#include "colors.h"
#include "pair.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

#ifdef __linux__
#include <limits.h>
#else
#define PATH_MAX 4096
#endif

static void
draw_status(const buffer *b,
            const char   *msg);

static int_array
find_line_matches(const buffer *b,
                  const str    *s);

PAIR_TYPE(int, int, int_pair);
PAIR_IMPL(int, int, int_pair);
DYN_ARRAY_TYPE(int_array, int_array_array);
DYN_ARRAY_TYPE(int_pair, int_pair_array);

static char_array g_cpy_buf = dyn_array_empty(char_array);

static void
append_cpy(buffer *b)
{
        dyn_array_append(g_cpy_buf, 0);
        str_overwrite(&b->cpy, g_cpy_buf.data);
        dyn_array_clear(g_cpy_buf);
}

static void
append_line_range_to_clipboard(line *ln, size_t from, size_t to) {
        if (!ln || from >= str_len(&ln->s)) return;
        if (to > str_len(&ln->s)) to = str_len(&ln->s);
        if (from >= to) return;

        for (size_t i = from; i < to; ++i) {
                dyn_array_append(g_cpy_buf, str_at(&ln->s, i));
        }
}

static void
copy_selection(buffer *b) {
        if (b->state != BS_SELECTION) {
                dyn_array_clear(g_cpy_buf);
                append_cpy(b);
                return;
        }

        dyn_array_clear(g_cpy_buf);

        size_t anchor_y = (size_t)b->sy;
        size_t anchor_x = (size_t)b->sx;
        size_t cursor_y = b->al;
        size_t cursor_x = b->cx;

        // normalize
        bool forward = (anchor_y < cursor_y) ||
                (anchor_y == cursor_y && anchor_x <= cursor_x);

        size_t start_y = forward ? anchor_y : cursor_y;
        size_t end_y   = forward ? cursor_y   : anchor_y;
        size_t start_x = forward ? anchor_x : cursor_x;
        size_t end_x   = forward ? cursor_x : anchor_x;

        // invalid line numbers
        if (start_y >= b->lns.len || end_y >= b->lns.len) {
                append_cpy(b);
                return;
        }

        if (start_y == end_y) {
                // single line selection
                line *ln = b->lns.data[start_y];
                append_line_range_to_clipboard(ln, start_x, end_x);
        }
        else {
                // multi-line selection

                // first (partial) line: start_x -> end of line
                {
                        line *ln = b->lns.data[start_y];
                        append_line_range_to_clipboard(ln, start_x, SIZE_MAX);
                        //dyn_array_append(g_cpy_buf, '\n');
                }

                // middle full lines
                for (size_t y = start_y + 1; y < end_y; ++y) {
                        line *ln = b->lns.data[y];
                        append_line_range_to_clipboard(ln, 0, SIZE_MAX);
                }

                // last (partial) line: beginning -> end_x
                {
                        line *ln = b->lns.data[end_y];
                        append_line_range_to_clipboard(ln, 0, end_x);
                }
        }

        append_cpy(b);
}

buffer *
buffer_alloc(window *parent)
{
        buffer *b = (buffer *)malloc(sizeof(buffer));

        b->filename    = str_create();
        b->lns         = dyn_array_empty(line_array);
        b->cx          = 0;
        b->cy          = 0;
        b->al          = 0;
        b->wish_col    = 0;
        b->hscrloff    = 0;
        b->vscrloff    = 0;
        b->parent      = parent;
        b->saved       = 1;
        b->state       = BS_NORMAL;
        b->last_search = str_create();
        b->cpy         = str_create();
        b->sy          = 0;
        b->sx          = 0;

        return b;
}

int
buffer_save(buffer *b)
{
        char_array content = dyn_array_empty(char_array);
        for (size_t i = 0; i < b->lns.len; ++i) {
                const line *ln = b->lns.data[i];
                for (size_t j = 0; j < ln->s.len; ++j) {
                        dyn_array_append(content, ln->s.chars[j]);
                }
        }
        dyn_array_append(content, 0);
        if (!write_file(str_cstr(&b->filename), content.data)) {
                perror("write_file");
                return 0;
        }

        b->saved = 1;
        draw_status(b, "saved");
        return 1;
}

buffer *
buffer_from_file(str filename, window *parent)
{
        buffer *b;

        b = buffer_alloc(parent);
        str_destroy(&b->filename);
        b->filename = filename;

        if (file_exists(str_cstr(&filename))) {
                char       *file_data;

                file_data = load_file(str_cstr(&filename));
                b->lns    = lines_of_cstr(file_data);
        } else {
                if (!create_file(str_cstr(&filename), 1)) {
                        perror("create_file");
                        return NULL;
                }
        }

        b->al = 0;

        return b;
}

static inline size_t
get_win_hight(buffer *b)
{
        return b->parent->h-1; // -1 for status line
}

static void
adjust_vscroll(buffer *b)
{
        size_t win_h = get_win_hight(b);

        if (b->cy < b->vscrloff)
                b->vscrloff = b->cy;
        else if (b->cy >= b->vscrloff + win_h)
                b->vscrloff = b->cy - win_h + 1;
}

static void
adjust_hscroll(buffer *b)
{
        size_t win_w = b->parent->w;

        if (b->cx < b->hscrloff)
                b->hscrloff = b->cx;
        else if (b->cx >= b->hscrloff + win_w)
                b->hscrloff = b->cx - win_w + 1;
}

static void
adjust_scroll(buffer *b)
{
        adjust_vscroll(b);
        adjust_hscroll(b);
}

static void
buffer_up(buffer *b)
{
        if (b->cy > 0) {
                --b->cy;
                --b->al;
        }

        const str *s = &b->lns.data[b->al]->s;
        if (b->wish_col > str_len(s)-1)
                b->cx = str_len(s)-1;
        else
                b->cx = b->wish_col;

        gotoxy(b->cx - b->hscrloff, b->cy - b->vscrloff);
        adjust_scroll(b);
}

static void
buffer_down(buffer *b)
{
        if (b->cy < b->lns.len-1) {
                ++b->cy;
                ++b->al;
        }

        const str *s = &b->lns.data[b->al]->s;
        if (b->wish_col > str_len(s)-1)
                b->cx = str_len(s)-1;
        else
                b->cx = b->wish_col;

        gotoxy(b->cx - b->hscrloff, b->cy - b->vscrloff);
        adjust_scroll(b);
}

static void
buffer_right(buffer *b)
{
        str *s = &b->lns.data[b->al]->s;

        if (b->cx == str_len(s)-1 && b->cy < b->lns.len-1) {
                b->cx = 0;
                ++b->cy;
                ++b->al;
        }
        else if (b->cx < str_len(s)-1)
                ++b->cx;
        b->wish_col = b->cx;
        gotoxy(b->cx - b->hscrloff, b->cy - b->vscrloff);
        adjust_scroll(b);
}

static void
buffer_left(buffer *b)
{
        if (b->cx == 0 && b->cy > 0) {
                b->cx = str_len(&b->lns.data[b->al-1]->s)-1;
                --b->cy;
                --b->al;
        }
        else if (b->cx > 0)
                --b->cx;
        b->wish_col = b->cx;
        gotoxy(b->cx - b->hscrloff, b->cy - b->vscrloff);
        adjust_scroll(b);
}

static void
buffer_eol(buffer *b)
{
        b->cx = str_len(&b->lns.data[b->al]->s)-1;
        b->wish_col = b->cx;
        gotoxy(b->cx - b->hscrloff, b->cy - b->vscrloff);
        adjust_scroll(b);
}

static void
buffer_bol(buffer *b)
{
        b->cx = 0;
        b->wish_col = b->cx;
        gotoxy(b->cx - b->hscrloff, b->cy - b->vscrloff);
        adjust_scroll(b);
}

static void
insert_char(buffer *b,
            char    ch,
            int     newline_advance)
{
        b->saved = 0;

        if (!b->lns.data) {
                char tmp[2] = {10, 0};
                dyn_array_append(b->lns, line_from(str_from(tmp)));
        }

        str_insert(&b->lns.data[b->al]->s, b->cx, ch);
        ++b->cx;

        if (ch == 10) {
                const char *rest;
                line       *newln;

                rest = str_cstr(&b->lns.data[b->al]->s)+b->cx;
                newln = line_from(str_from(rest));

                dyn_array_insert_at(b->lns, b->al+1, newln);
                str_cut(&b->lns.data[b->al]->s, b->cx);

                if (newline_advance) {
                        b->cx = 0;
                        ++b->cy;
                        ++b->al;
                }
        }

        b->wish_col = b->cx;
        adjust_scroll(b);
}

static void
del_selection(buffer *b)
{
        if (b->state != BS_SELECTION)
                return;

        size_t anchor_y = (size_t)b->sy;
        size_t anchor_x = (size_t)b->sx;
        size_t cursor_y = b->al;
        size_t cursor_x = b->cx;

        // normalize
        bool forward = (anchor_y < cursor_y) ||
                (anchor_y == cursor_y && anchor_x <= cursor_x);

        size_t start_y = forward ? anchor_y : cursor_y;
        size_t end_y   = forward ? cursor_y   : anchor_y;
        size_t start_x = forward ? anchor_x : cursor_x;
        size_t end_x   = forward ? cursor_x : anchor_x;

        if (start_y >= b->lns.len || end_y >= b->lns.len)
                goto cleanup;

        b->saved = 0;

        if (start_y == end_y) {
                // single-line deletion
                line *ln = b->lns.data[start_y];

                size_t remove_count = end_x - start_x;
                for (size_t i = 0; i < remove_count; ++i)
                        str_rm(&ln->s, start_x);

                b->cx = start_x;
                b->al = start_y;
                b->cy = start_y;
        } else {
                // multi-line deletion

                // first line
                {
                        line *first = b->lns.data[start_y];
                        size_t len_first = str_len(&first->s);
                        if (start_x < len_first) {
                                while (str_len(&first->s) > start_x)
                                        str_rm(&first->s, start_x);
                        }
                }

                // last line
                {
                        line *last = b->lns.data[end_y];
                        for (size_t i = 0; i < end_x; ++i)
                                str_rm(&last->s, 0);
                }

                // delete all middle lines
                size_t lines_to_remove = end_y - start_y - 1;
                for (size_t i = 0; i < lines_to_remove; ++i) {
                        line_free(b->lns.data[start_y + 1]);
                        dyn_array_rm_at(b->lns, start_y + 1);
                }

                // join the first and last lines
                line *first = b->lns.data[start_y];
                line *last  = b->lns.data[start_y + 1];

                str_concat(&first->s, str_cstr(&last->s));
                line_free(last);
                dyn_array_rm_at(b->lns, start_y + 1);

                // place cursor at the join point
                b->cx = start_x;
                b->al = start_y;
                b->cy = start_y;
        }

 cleanup:
        b->wish_col = b->cx;
        b->state = BS_NORMAL;
        adjust_scroll(b);
}

static int
del_char(buffer *b)
{
        if (b->state == BS_SELECTION) {
                del_selection(b);
                return 1;
        }

        line *ln;
        int   newline;

        ln       = b->lns.data[b->al];
        newline  = 0;
        b->saved = 0;

        if (ln->s.chars[b->cx] == 10) {
                newline = 1;
                if (b->al < b->lns.len-1) {
                        str *s = &ln->s;
                        str_concat(s, str_cstr(&b->lns.data[b->al+1]->s));
                        line_free(b->lns.data[b->al+1]);
                        dyn_array_rm_at(b->lns, b->al+1);
                } else {
                        return 0;
                }
        }

        str_rm(&ln->s, b->cx);
        if (b->cx > str_len(&ln->s)-1)
                b->cx = str_len(&ln->s)-1;

        adjust_scroll(b);
        return newline;
}

static int
backspace(buffer *b)
{
        line *ln;
        int   newline;

        ln       = b->lns.data[b->al];
        newline  = 0;
        b->saved = 0;

        if (b->cx == 0) {
                if (b->al == 0)
                        return 0;
                line   *prevln     = b->lns.data[b->al-1];
                size_t  prevln_len = str_len(&prevln->s);

                str_rm(&prevln->s, prevln_len-1);
                str_concat(&prevln->s, str_cstr(&ln->s));
                line_free(b->lns.data[b->al]);
                dyn_array_rm_at(b->lns, b->al);

                --b->al;
                b->cx = prevln_len-1;
                --b->cy;

                adjust_scroll(b);
                return 1;
        }

        buffer_left(b);

        str_rm(&ln->s, b->cx);
        if (b->cx > str_len(&ln->s)-1)
                b->cx = str_len(&ln->s)-1;

        adjust_scroll(b);
        return newline;
}

static void
tab(buffer *b)
{
        for (size_t i = 0; i < 8; ++i)
                insert_char(b, ' ', 1);
}

static void
delete_until_eol(buffer *b)
{
        line *ln;
        const str *s;

        ln = b->lns.data[b->al];
        s  = &ln->s;

        for (size_t i = b->cx; i < str_len(s)-1; ++i)
                dyn_array_append(g_cpy_buf, str_at(s, i));

        str_cut(&ln->s, b->cx);
        str_insert(&ln->s, b->cx, 10);

        append_cpy(b);
}

static void
jump_to_first_char(buffer *b)
{
        const str  *s;
        const char *sraw;

        s    = &b->lns.data[b->al]->s;
        sraw = str_cstr(s);

        for (size_t i = 0; i < str_len(s); ++i) {
                if (sraw[i] != ' ' && sraw[i] != '\n' && sraw[i] != '\t' && sraw[i] != '\r') {
                        b->cx = i;
                        break;
                }
        }

        b->wish_col = b->cx;

        gotoxy(b->cx - b->hscrloff, b->cy - b->vscrloff);
}

static void
jump_to_top_of_buffer(buffer *b)
{
        if (b->lns.len == 0) // not sure if this is required, but doesn't hurt
                return;

        b->cy = b->lns.len-1;
        b->cx = 0;
        b->al = b->lns.len-1;
        adjust_scroll(b);
}

static void
jump_to_bottom_of_buffer(buffer *b)
{
        b->cy = 0;
        b->cx = 0;
        b->al = 0;
        adjust_scroll(b);
}

static void
prev_paragraph(buffer *b)
{
        size_t nextln;

        nextln = b->cy;

        for (int i = b->cy-1; i >= 0; --i) {
                const line *l  = b->lns.data[i];
                const line *l2 = b->lns.data[i+1];
                nextln         = i;
                if (str_len(&l->s) == 1 && l->s.chars[0] == 10) {
                        if (i > 0 && l2 && l2->s.chars[0] == 10)
                                continue;
                        break;
                }
        }

        b->cy = nextln;
        b->cx = 0;
        b->al = nextln;
        adjust_scroll(b);
}

static void
next_paragraph(buffer *b)
{
        size_t nextln;

        nextln = b->cy;

        for (size_t i = b->cy+1; i < b->lns.len; ++i) {
                const line *l  = b->lns.data[i];
                const line *l2 = b->lns.data[i+1];
                nextln         = i;
                if (str_len(&l->s) == 1 && l->s.chars[0] == 10) {
                        if (i < b->lns.len && l2 && l2->s.chars[0] == 10)
                                continue;
                        break;
                }
        }

        b->cy = nextln;
        b->cx = 0;
        b->al = nextln;
        adjust_scroll(b);
}

static void
kill_line(buffer *b)
{
        line *ln;
        const str *s;

        if (b->lns.len <= 0)
                return;

        ln = b->lns.data[b->al];
        s  = &ln->s;

        for (size_t i = 0; i < str_len(s); ++i)
                dyn_array_append(g_cpy_buf, str_at(s, i));

        line_free(ln);
        dyn_array_rm_at(b->lns, b->al);

        if (b->al > b->lns.len-1) {
                --b->al;
                --b->cy;
        }

        b->cx       = 0;
        b->wish_col = 0;

        adjust_scroll(b);

        append_cpy(b);
}

static void
jump_next_word(buffer *b)
{
        const line *ln;
        const str  *s;
        const char *sraw;
        int         hitchars;
        size_t      i;

        ln       = b->lns.data[b->al];
        s        = &ln->s;
        sraw     = str_cstr(s);
        hitchars = 0;
        i        = b->cx;

        if (str_len(s) <= 0)
                return;

        while (i < str_len(s)) {
                if (isalnum(sraw[i]))
                        hitchars = 1;
                else if (hitchars)
                        break;
                ++i;
        }

        if (i == str_len(s))
                b->cx = str_len(s)-1;
        else
                b->cx = i;

        b->wish_col = b->cx;
}

static void
jump_prev_word(buffer *b)
{
        const line *ln;
        const str  *s;
        const char *sraw;
        int         hitchars;
        size_t      i;

        ln       = b->lns.data[b->al];
        s        = &ln->s;
        sraw     = str_cstr(s);
        hitchars = 0;
        i        = b->cx-1;

        if (str_len(s) == 0 || b->cx == 0)
                return;

        while (i > 0) {
                if (isalnum(sraw[i]))
                        hitchars = 1;
                else if (hitchars)
                        break;
                --i;
        }

        b->cx = i;

        if (!isalnum(sraw[b->cx]))
                ++b->cx;

        b->wish_col = b->cx;
}

static void
del_word(buffer *b)
{
        line        *ln;
        str         *s;
        const char  *sraw;
        int          hitchars;
        size_t       i;

        ln       = b->lns.data[b->al];
        s        = &ln->s;
        sraw     = str_cstr(s);
        hitchars = 0;
        i        = b->cx;

        while (i < str_len(s)) {
                if (sraw[i] == 10)
                        break;
                if (isalnum(sraw[i]))
                        hitchars = 1;
                else if (!isalnum(sraw[i]) && hitchars)
                        break;
                dyn_array_append(g_cpy_buf, str_at(s, i));
                str_rm(s, i);
        }

        if (hitchars)
                append_cpy(b);
}

static void
center_view(buffer *b)
{
        int rows = b->parent->h;
        int vertical_offset = b->cy - (rows/2);
        if (vertical_offset < 0)
                vertical_offset = 0;

        int max_offset = b->lns.len - rows;
        if (max_offset < 0)
                max_offset = 0;

        //if (vertical_offset > max_offset)
        //        vertical_offset = max_offset;

        b->vscrloff = vertical_offset;
        adjust_scroll(b);
}

int_pair_array
find_all_matches_in_buffer(buffer *b)
{
        int_pair_array pairs = dyn_array_empty(int_pair_array);
        for (size_t i = 0; i < b->lns.len; ++i) {
                int_array verts = find_line_matches(b, &b->lns.data[i]->s);
                for (size_t j = 0; j < verts.len; ++j)
                        dyn_array_append(pairs, int_pair_create(i, verts.data[j]));
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
        int         old_cx;
        int         old_cy;
        int         old_al;
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
                int_pair_array pairs = find_all_matches_in_buffer(b);

                if (adjust) {
                        step = 0;

                        for (size_t i = 0; i < pairs.len; ++i) {
                                if (pairs.data[i].l < b->al)
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

                if (pairs.len > 0 && step < pairs.len) {
                        b->al = pairs.data[step].l;
                        b->cy = pairs.data[step].l;
                        b->cx = pairs.data[step].r;
                        adjust_scroll(b);
                }

                center_view(b);
                buffer_dump(b);

                gotoxy(0, b->parent->h);
                clear_line(0, b->parent->h);
                printf("Search: %s", str_cstr(input));
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
                                if (pairs.len > 0 && step < pairs.len) {
                                        b->al = pairs.data[step].l;
                                        b->cy = pairs.data[step].l;
                                        b->cx = pairs.data[step].r;
                                }
                                break;
                        } else {
                                b->cx = old_cx; b->cy = old_cy; b->al = old_al;
                                adjust = 1;
                                str_append(input, ch);
                        }
                } else if (ty == INPUT_TYPE_CTRL && ch == CTRL_S) {
                        if (step < pairs.len-1)
                                ++step;
                } else if (ty == INPUT_TYPE_CTRL && ch == CTRL_R) {
                        if (step > 0)
                                --step;
                } else {
                        b->cx = old_cx; b->cy = old_cy; b->al = old_al;
                        break;
                }
        }

        b->state = BS_NORMAL;

        adjust_scroll(b);
}

static int
paste(buffer *b)
{
        int newline;

        newline = 0;

        for (size_t i = 0; i < b->cpy.len; ++i) {
                insert_char(b, b->cpy.chars[i], 1);
                if (b->cpy.chars[i] == '\n')
                        newline = 1;
        }

        return newline;
}

static void
combine_lines(buffer *b)
{
        line   *l0;
        line   *l1;
        str    *s0;
        str    *s1;
        size_t len;

        if (b->al >= b->lns.len-1)
                return;

        l0 = b->lns.data[b->al];
        s0 = &l0->s;
        l1 = b->lns.data[b->al+1];
        s1 = &l1->s;
        len = str_len(s0);

        s0->chars[s0->len-1] = ' ';
        str_trim_before(s1);
        str_concat(s0, str_cstr(s1));
        dyn_array_rm_at(b->lns, b->al+1);

        b->cx = len-1;
        b->wish_col = b->cx;
}

static void
page_down(buffer *b)
{
        size_t h;

        h = b->parent->h;

        if (b->al + h > b->lns.len) {
                b->al = b->lns.len-1;
                b->cy = b->lns.len-1;
        } else {
                b->al += h;
                b->cy += h;
        }

        adjust_scroll(b);
}

static void
page_up(buffer *b)
{
        size_t h;

        h = b->parent->h;

        if ((int)b->al - (int)h < 0) {
                b->al = 0;
                b->cy = 0;
        } else {
                b->al -= h;
                b->cy -= h;
        }

        adjust_scroll(b);
}

static void
selection(buffer *b)
{
        if (b->state == BS_NORMAL)
                b->state = BS_SELECTION;
        else if (b->state == BS_SELECTION)
                b->state = BS_NORMAL;
        else
                return;

        b->sy = b->al;
        b->sx = b->cx;
}

static void
cancel(buffer *b)
{
        b->state = BS_NORMAL;
}

// entrypoint
buffer_proc
buffer_process(buffer     *b,
               input_type  ty,
               char        ch)
{
        static void (*movement_ar[])(buffer *) = {
                buffer_up,
                buffer_down,
                buffer_right,
                buffer_left,
                buffer_eol,
                buffer_bol,
        };

        switch (ty) {
        case INPUT_TYPE_CTRL: {
                if (ch == CTRL_N) {
                        buffer_down(b);
                        return BP_MOV;
                } else if (ch == CTRL_P) {
                        buffer_up(b);
                        return BP_MOV;
                } else if (ch == CTRL_F) {
                        buffer_right(b);
                        return BP_MOV;
                } else if (ch == CTRL_B) {
                        buffer_left(b);
                        return BP_MOV;
                } else if (ch == CTRL_E) {
                        buffer_eol(b);
                        return BP_MOV;
                } else if (ch == CTRL_A) {
                        buffer_bol(b);
                        return BP_MOV;
                } else if (ch == CTRL_D) {
                        return del_char(b) ? BP_INSERTNL : BP_INSERT;
                } else if (TAB(ch)) {
                        tab(b);
                        return BP_INSERT;
                } else if (ch == CTRL_K) {
                        delete_until_eol(b);
                        return BP_INSERT;
                } else if (ch == CTRL_O) {
                        insert_char(b, 10, 0);
                        --b->cx;
                        return BP_INSERTNL;
                } else if (ch == CTRL_H) {
                        return backspace(b) ? BP_INSERTNL : BP_INSERT;
                } else if (ch == CTRL_S || ch == CTRL_R) {
                        search(b, ch == CTRL_R);
                        return BP_MOV;
                } else if (ch == CTRL_L) {
                        center_view(b);
                        return BP_MOV;
                } else if (ch == CTRL_Y) {
                        return paste(b) ? BP_INSERTNL : BP_INSERT;
                } else if (ch == CTRL_V) {
                        page_down(b);
                        return BP_INSERTNL;
                } else if (ch == CTRL_G) {
                        cancel(b);
                        return BP_MOV;
                }

        } break;
        case INPUT_TYPE_ALT: {
                if (ch == 'm') {
                        jump_to_first_char(b);
                        return BP_MOV;
                } else if (ch == '<') {
                        jump_to_bottom_of_buffer(b);
                        return BP_MOV;
                } else if (ch == '>') {
                        jump_to_top_of_buffer(b);
                        return BP_MOV;
                } else if (ch == '{') {
                        prev_paragraph(b);
                        return BP_MOV;
                } else if (ch == '}') {
                        next_paragraph(b);
                        return BP_MOV;
                } else if (ch == 'k') {
                        kill_line(b);
                        return BP_INSERTNL;
                } else if (ch == 'f') {
                        jump_next_word(b);
                        return BP_MOV;
                } else if (ch == 'b') {
                        jump_prev_word(b);
                        return BP_MOV;
                } else if (ch == 'd') {
                        del_word(b);
                        return BP_INSERT;
                } else if (ch == 'j') {
                        combine_lines(b);
                        return BP_INSERTNL;
                } else if (ch == 'v') {
                        page_up(b);
                        return BP_INSERTNL;
                } else if (ch == 'w') {
                        copy_selection(b);
                        cancel(b);
                        return BP_MOV;
                }
        } break;
        case INPUT_TYPE_ARROW: {
                movement_ar[ch-'A'](b);
                return BP_MOV;
        } break;
        case INPUT_TYPE_NORMAL: {
                if (BACKSPACE(ch))
                        return backspace(b) ? BP_INSERTNL : BP_INSERT;
                else if (ch == 0) { // ctrl+space
                        selection(b);
                        return BP_INSERTNL;
                }
                insert_char(b, ch, 1);
                return ch == 10 ? BP_INSERTNL : BP_INSERT;
        } break;
        default: break;
        }

        return BP_NOP;
}

static void
show_whitespace(const buffer *b,
                const str    *s,
                int           eol)
{
        int         space;
        const char *sraw;

        space = -1;
        sraw  = str_cstr(s);

        if (eol <= -1)
                return;
        else {
                printf(GRAY);
                for (size_t i = 0; i < str_len(s)-eol-1; ++i)
                        putchar('-');
                printf(RESET);
        }
}

static int_array
find_line_matches(const buffer *b,
                  const str    *s)
{
        assert(b->state == BS_SEARCH);

        static int_array  ar;
        const char       *sraw;
        const char       *query;

        ar    = dyn_array_empty(int_array);
        sraw  = str_cstr(s);
        query = str_cstr(&b->last_search);

        if (!sraw || !query || !str_len(&b->last_search))
                return ar;

        for (size_t i = 0; i < str_len(s); ++i) {
                if (!memcmp(query, sraw+i, str_len(&b->last_search))) {
                        dyn_array_append(ar, i);
                        i += str_len(&b->last_search)-1;
                }
        }

        return ar;
}

static void
drawln(const buffer *b,
       const line   *l,
       int           lineno)
{
        const str  *s;
        const char *sraw;
        int         eol;
        size_t      n;

        s    = &l->s;
        n    = str_len(s);
        sraw = str_cstr(s);
        eol  = -1;

        for (int i = n-1; i >= 0; --i) {
                if (sraw[i] == '\n') continue;
                if (sraw[i] == ' ')  eol = i;
                else                 break;
        }

        if (b->state == BS_SEARCH) {
                int_array matches = find_line_matches(b, s);

                for (size_t i = 0; i < str_len(s); ++i) {
                        if (matches.len > 0 && i == matches.data[0]) {
                                if (b->cx >= i && b->cx <= matches.data[0] + str_len(&b->last_search)
                                        && b->al == lineno)
                                        printf(INVERT BOLD ORANGE "%s" RESET, str_cstr(&b->last_search));
                                else
                                        printf(INVERT DIM YELLOW "%s" RESET, str_cstr(&b->last_search));
                                dyn_array_rm_at(matches, 0);
                                i += str_len(&b->last_search)-1;
                        } else {
                                putchar(str_at(s, i));
                        }
                }

                dyn_array_free(matches);
                goto done;
        } else if (b->state == BS_SELECTION) {
                int selection_active = 1;

                size_t anchor_y = b->sy;
                size_t cursor_y = b->al;
                size_t anchor_x = b->sx;
                size_t cursor_x = b->cx;

                int forward = (anchor_y < cursor_y) ||
                        (anchor_y == cursor_y && anchor_x <= cursor_x);

                size_t start_y = forward ? anchor_y : cursor_y;
                size_t end_y   = forward ? cursor_y : anchor_y;
                size_t start_x = forward ? anchor_x : cursor_x;
                size_t end_x   = forward ? cursor_x : anchor_x;

                int is_start_line  = (lineno == start_y);
                int is_end_line    = (lineno == end_y);
                int is_middle_line = (lineno > start_y && lineno < end_y);

                for (size_t i = 0; i < str_len(s); ++i) {
                        bool in_selection = false;

                        if (is_middle_line) {
                                // whole line is selected
                                in_selection = true;
                        } else if (is_start_line && is_end_line) {
                                // selection starts and ends on same line
                                in_selection = (i >= start_x && i < end_x);
                        } else if (is_start_line) {
                                // only beginning of selection
                                in_selection = (i >= start_x);
                        } else if (is_end_line) {
                                // only end of selection
                                in_selection = (i < end_x);
                        }

                        if (in_selection) {
                                printf(INVERT "%c", str_at(s, i));
                        } else {
                                printf(RESET "%c", str_at(s, i));
                        }
                }

                goto done;
        }

        if (eol == -1) {
                printf("%s", sraw);
        } else {
                for (size_t i = 0; i < eol; ++i) {
                        putchar(sraw[i]);
                }
        }

done:
        show_whitespace(b, s, eol);
}

static void
draw_status(const buffer *b,
            const char   *msg)
{
        char   buf[PATH_MAX + 32];
        size_t len;

        len = 0;

        gotoxy(0, b->parent->h);

        printf(INVERT);

        sprintf(buf, "%s:%zu:%zu%s",
                str_cstr(&b->filename),
                b->cy+1,
                b->cx+1,
                !b->saved ? "*" : "");
        printf("%s", buf);
        len += strlen(buf);

        if (msg) {
                sprintf(buf, " [%s" RESET INVERT "]", msg);
                printf("%s", buf);
                len += strlen(buf);
        }

        for (size_t i = len; i < b->parent->w; ++i)
                putchar(' ');

        printf(RESET);

        gotoxy(b->cx - b->hscrloff, b->cy - b->vscrloff);
        fflush(stdout);
}

void
buffer_dump_xy(const buffer *b)
{
        const line *l;
        const str  *s;

        l = b->lns.data[b->al];
        s = &l->s;

        if (!s)
                return;

        size_t screen_y = b->cy - b->vscrloff;

        gotoxy(0, screen_y);
        printf("\x1b[K"); // clear rest of line
        drawln(b, l, b->al);

        gotoxy(b->cx - b->hscrloff, screen_y);
        draw_status(b, NULL);
}

void
buffer_dump(const buffer *b)
{
        clear_terminal();

        size_t start = b->vscrloff;
        size_t end   = start + b->parent->h;
        if (end > b->lns.len) end = b->lns.len;

        for (size_t i = start; i < end; ++i) {
                const line *l = b->lns.data[i];
                if (!l) break;

                gotoxy(0, i - b->vscrloff);
                printf("\x1b[K");
                drawln(b, l, i);
        }

        gotoxy(b->cx - b->hscrloff, b->cy - b->vscrloff);
        draw_status(b, NULL);
}

