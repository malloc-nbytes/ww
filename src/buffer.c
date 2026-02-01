#include "buffer.h"
#include "io.h"
#include "term.h"
#include "utils.h"
#include "window.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

buffer *
buffer_alloc(window *parent)
{
        buffer *b = (buffer *)malloc(sizeof(buffer));

        b->filename = str_create();
        b->lns      = dyn_array_empty(line_array);
        b->cx       = 0;
        b->cy       = 0;
        b->al       = 0;
        b->wish_col = 0;
        b->hscrloff = 0;
        b->vscrloff = 0;
        b->parent   = parent;

        return b;
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
                b->al     = 0;
        }

        return b;
}

static void
adjust_vscroll(buffer *b)
{
        size_t win_h = b->parent->h;

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

        gotoxy(b->cx, b->cy);
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

        gotoxy(b->cx, b->cy);
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
        gotoxy(b->cx, b->cy);
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
        gotoxy(b->cx, b->cy);
        adjust_scroll(b);
}

static void
buffer_eol(buffer *b)
{
        b->cx = str_len(&b->lns.data[b->al]->s)-1;
        b->wish_col = b->cx;
        gotoxy(b->cx, b->cy);
        adjust_scroll(b);
}

static void
buffer_bol(buffer *b)
{
        b->cx = 0;
        b->wish_col = b->cx;
        gotoxy(b->cx, b->cy);
        adjust_scroll(b);
}

static void
insert_char(buffer *b, char ch)
{
        str_insert(&b->lns.data[b->al]->s, b->cx, ch);
        ++b->cx;

        if (ch == 10) {
                const char *rest = str_cstr(&b->lns.data[b->al]->s)+b->cx;
                line *newln = line_from(9999, str_from(rest));
                dyn_array_insert_at(b->lns, b->al+1, newln);
                str_cut(&b->lns.data[b->al]->s, b->cx);
                b->cx = 0;
                ++b->cy;
                ++b->al;
        }

        b->wish_col = b->cx;
        adjust_scroll(b);
}

void
buffer_dump_xy(const buffer *b)
{
        const str *s;

        s = &b->lns.data[b->al]->s;
        clear_line(0, b->cy);
        printf("%s", str_cstr(s));
        gotoxy(b->cx, b->cy);
        fflush(stdout);
}

static int
del_char(buffer *b)
{
        line *ln;
        int   newline;

        ln      = b->lns.data[b->al];
        newline = 0;

        if (ln->s.chars[b->cx] == 10) {
                str *s = &ln->s;
                newline = 1;
                str_concat(s, str_cstr(&b->lns.data[b->al+1]->s));
                line_free(b->lns.data[b->al+1]);
                dyn_array_rm_at(b->lns, b->al+1);
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

        ln      = b->lns.data[b->al];
        newline = 0;

        if (b->cx == 0) {
                if (b->al == 0)
                        return 0;
                line *prevln = b->lns.data[b->al-1];
                size_t prevln_len = str_len(&prevln->s);
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
                insert_char(b, ' ');
}

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
                        movement_ar[1](b);
                        return BP_MOV;
                } else if (ch == CTRL_P) {
                        movement_ar[0](b);
                        return BP_MOV;
                } else if (ch == CTRL_F) {
                        movement_ar[2](b);
                        return BP_MOV;
                } else if (ch == CTRL_B) {
                        movement_ar[3](b);
                        return BP_MOV;
                } else if (ch == CTRL_E) {
                        movement_ar[4](b);
                        return BP_MOV;
                } else if (ch == CTRL_A) {
                        movement_ar[5](b);
                        return BP_MOV;
                } else if (ch == CTRL_D) {
                        return del_char(b) ? BP_INSERTNL : BP_INSERT;
                } else if (TAB(ch)) {
                        tab(b);
                        return BP_INSERT;
                }
        } break;
        case INPUT_TYPE_ARROW: {
                movement_ar[ch-'A'](b);
                return BP_MOV;
        } break;
        case INPUT_TYPE_NORMAL: {
                if (BACKSPACE(ch)) {
                        return backspace(b) ? BP_INSERTNL : BP_INSERT;
                }
                insert_char(b, ch);
                return ch == 10 ? BP_INSERTNL : BP_INSERT;
        } break;
        default: break;
        }

        return BP_NOP;
}

void
buffer_dump(const buffer *b)
{
        clear_terminal();

        size_t start;
        size_t end;

        start = b->vscrloff;
        start = 0;

        for (size_t i = start; i < b->lns.len; ++i) {
                line *l = b->lns.data[i];
                printf("%s", str_cstr(&l->s));
        }

        gotoxy(b->cx, b->cy);
        fflush(stdout);
}

