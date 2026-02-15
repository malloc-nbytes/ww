#include "buffer.h"
#include "io.h"
#include "term.h"
#include "utils.h"
#include "window.h"
#include "colors.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

static void
draw_status(const buffer *b,
            const char   *msg);

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
        b->saved    = 1;

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

static int
del_char(buffer *b)
{
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

        ln = b->lns.data[b->al];

        str_cut(&ln->s, b->cx);
        str_insert(&ln->s, b->cx, 10);
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
                } else if (ch == CTRL_K) {
                        delete_until_eol(b);
                        return BP_INSERT;
                } else if (ch == CTRL_O) {
                        insert_char(b, 10, 0);
                        --b->cx;
                        return BP_INSERTNL;
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
                insert_char(b, ch, 1);
                return ch == 10 ? BP_INSERTNL : BP_INSERT;
        } break;
        default: break;
        }

        return BP_NOP;
}

static void
drawln(const str *s)
{
        int         space;
        const char *sraw;

        space = -1;
        sraw  = str_cstr(s);

        for (int i = str_len(s)-1; i >= 0; --i) {
                if (sraw[i] == 10)
                        continue;
                if (sraw[i] != ' ')
                        break;
                space = i;
        }

        if (space == -1)
                printf("%s", sraw);
        else {
                for (size_t i = 0; i < space; ++i)
                        putchar(sraw[i]);
                printf(GRAY);
                for (size_t i = 0; i < str_len(s)-space-1; ++i)
                        putchar('-');
                printf(RESET);
        }
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
                b->cx+1,
                b->cy+1,
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
        const str *s = &b->lns.data[b->al]->s;
        if (!s)
                return;

        size_t screen_y = b->cy - b->vscrloff;

        gotoxy(0, screen_y);
        printf("\x1b[K"); // clear rest of line
        //printf("%s", str_cstr(s));
        drawln(s);

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
                // printf("%s", str_cstr(&l->s));
                drawln(&l->s);
        }

        gotoxy(b->cx - b->hscrloff, b->cy - b->vscrloff);
        draw_status(b, NULL);
}

