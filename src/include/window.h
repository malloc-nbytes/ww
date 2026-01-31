#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include "buffer.h"

#include <stddef.h>

typedef struct {
        buffer        *ab;
        bufferp_array  bfrs;
        size_t         w;
        size_t         h;
} window;

window window_create(size_t w, size_t h);
void   window_add_buffer(window *win, buffer *b, int make_curr);
void   window_handle(window *win);

#endif // WINDOW_H_INCLUDED
