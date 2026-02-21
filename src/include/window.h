#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include "buffer.h"

#include <stddef.h>

#define WINCMD_SPCAMT        "set-space-amt"
#define WINCMD_KILLBUF       "kill-buffer"
#define WINCMD_SWTCHBUF      "switch-buffer"
#define WINCMD_COMP          "compile"
#define WINCMD_SAVEBUF       "save-buffer"
#define WINCMD_EXIT          "exit"
#define WINCMD_COPYBUFTOCLIP "copybuffer-to-clipboard"
#define WINCMD_TABMODE       "tab-mode"
#define WINCMD_SPACEMODE     "space-mode"
#define WINCMD_REPLACE       "replace"
#define WINCMD_HELP          "help"
#define WINCMD_FINDFILE      "find-file"
#define WINCMDS { \
        WINCMD_SPCAMT, \
        WINCMD_KILLBUF, \
        WINCMD_SWTCHBUF, \
        WINCMD_COMP, \
        WINCMD_SAVEBUF, \
        WINCMD_EXIT, \
        WINCMD_COPYBUFTOCLIP, \
        WINCMD_TABMODE, \
        WINCMD_SPACEMODE, \
        WINCMD_REPLACE, \
        WINCMD_HELP, \
        WINCMD_FINDFILE, \
}

typedef struct window {
        buffer        *ab;      // active buffer (pointer)
        size_t         abi;     // active buffer index
        bufferp_array  bfrs;    // buffers
        size_t         w;       // window width
        size_t         h;       // window height
        char          *compile; // current compile command
        buffer        *pb;      // previous buffer (pointer)
        size_t         pbi;     // previous buffer index
} window;

window window_create(size_t w, size_t h);
void   window_add_buffer(window *win, buffer *b, int make_curr);
void   window_handle(window *win);
void   window_open_help_buffer(window *win);

#endif // WINDOW_H_INCLUDED
