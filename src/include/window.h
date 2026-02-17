#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include "buffer.h"

#include <stddef.h>

typedef struct window {
        buffer        *ab;   // active buffer (pointer)
        size_t         abi;  // active buffer index
        bufferp_array  bfrs; // buffers
        size_t         w;    // window width
        size_t         h;    // window height
        char          *compile; // current compile command
        buffer        *pb; // previous buffer (pointer)
        size_t         pbi; // previous buffer index
} window;

window window_create(size_t w, size_t h);
void   window_add_buffer(window *win, buffer *b, int make_curr);
void   window_handle(window *win);

#endif // WINDOW_H_INCLUDED
