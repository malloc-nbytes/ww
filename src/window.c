#include "window.h"
#include "term.h"

#include <assert.h>
#include <stdio.h>

window
window_create(size_t w, size_t h)
{
        return (window) {
                .ab   = NULL,
                .abi  = 0,
                .bfrs = dyn_array_empty(bufferp_array),
                .w    = w,
                .h    = h,
        };
}

void
window_add_buffer(window *win,
                  buffer *b,
                  int     make_curr)
{
        dyn_array_append(win->bfrs, b);
        if (make_curr) {
                win->ab  = b;
                win->abi = win->bfrs.len-1;
        }
}

static void
close_buffer(window *win)
{
        dyn_array_rm_at(win->bfrs, win->abi);
        if (win->bfrs.len == 0) {
                win->ab = NULL;
                win->abi = 0;
        }
        else if (win->abi > win->bfrs.len-1)
                --win->abi;
}

static void
ctrlx(window *win)
{
        char        ch;
        input_type  ty;

        switch (ty = get_input(&ch)) {
        case INPUT_TYPE_NORMAL:
                if (ch == 'q')
                        close_buffer(win);
        default: break;
        }
}

void
window_handle(window *win)
{
        assert(win->ab);

        buffer_dump(win->ab);
        gotoxy(win->ab->cx, win->ab->cy);
        fflush(stdout);

        while (1) {
                if (!win->ab)
                        break;

                char        ch;
                input_type  ty;
                buffer_proc bproc;

                ty = get_input(&ch);

                if (ty == INPUT_TYPE_ALT)
                        assert(0);
                else if (ty == INPUT_TYPE_CTRL && ch == CTRL_X)
                        ctrlx(win);
                else {
                        bproc = buffer_process(win->ab, ty, ch);

                        if (bproc == BP_INSERT)
                                buffer_dump_xy(win->ab);
                        else if (bproc == BP_INSERTNL)
                                buffer_dump(win->ab);
                        else if (bproc == BP_MOV)
                                buffer_dump(win->ab);
                }
        }
}
