#include "buffer.h"
#include "io.h"
#include "term.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

buffer *
buffer_alloc(void)
{
        buffer *b = (buffer *)malloc(sizeof(buffer));

        b->filename = str_create();
        b->lns      = dyn_array_empty(line_array);
        b->cx       = 0;
        b->cy       = 0;
        b->al       = 0;

        return b;
}

buffer *
buffer_from_file(str filename)
{
        buffer *b;

        b = buffer_alloc();
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
buffer_up(buffer *b)
{
        if (b->cy > 0) {
                --b->cy;
                --b->al;
        }

        const str *s = &b->lns.data[b->al]->s;
        if (b->cx > str_len(s)-1)
                b->cx = str_len(s)-1;
}

static void
buffer_down(buffer *b)
{
        if (b->cy < b->lns.len-1) {
                ++b->cy;
                ++b->al;
        }

        const str *s = &b->lns.data[b->al]->s;
        if (b->cx > str_len(s)-1)
                b->cx = str_len(s)-1;
}

static void
buffer_right(buffer *b)
{
        str *s = &b->lns.data[b->al]->s;

        if (b->cx < str_len(s)-1)
                ++b->cx;
}

static void
buffer_left(buffer *b)
{
        if (b->cx > 0)
                --b->cx;
}

buffer_proc
buffer_process(buffer     *b,
               input_type  ty,
               char        ch)
{
        static void (*movement_ar[4])(buffer *) = {
                buffer_up,
                buffer_down,
                buffer_right,
                buffer_left,
        };

        switch (ty) {
        case INPUT_TYPE_CTRL: {
                if (ch == 'n')
                        movement_ar[1](b);
                else if (ch == 'p')
                        movement_ar[0](b);
                else if (ch == 'f')
                        movement_ar[2](b);
                else if (ch == 'b')
                        movement_ar[3](b);
                gotoxy(b->cx, b->cy);
                fflush(stdout);
        } break;
        case INPUT_TYPE_ARROW:
                movement_ar[ch-'A'](b);
                gotoxy(b->cx, b->cy);
                fflush(stdout);
                return BP_MOV;
        default: break;
        }

        return BP_NOP;
}

void
buffer_dump(const buffer *b)
{
        for (size_t i = 0; i < b->lns.len; ++i) {
                line *l = b->lns.data[i];
                printf("%s", str_cstr(&l->s));
        }
}
