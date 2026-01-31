#include "window.h"
#include "term.h"

#include <assert.h>
#include <stdio.h>

window
window_create(size_t w, size_t h)
{
        return (window) {
                .ab   = NULL,
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
        if (make_curr)
                win->ab = b;
}

void
window_handle(window *win)
{
        assert(win->ab);

        buffer_dump(win->ab);

        while (1) {
                char        ch;
                input_type  ty;
                buffer_proc proc;

                ty = get_input(&ch);

                if (ty == INPUT_TYPE_ALT) assert(0);
                if (ty == INPUT_TYPE_CTRL
                    && ch == 'x') assert(0);

                proc = buffer_process(win->ab, ty, ch);

                if (proc == BP_UPDATE) assert(0);
                else if (proc == BP_MOV) assert(0);
        }
}
