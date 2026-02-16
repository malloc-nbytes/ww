#ifndef BUFFER_H_INUCLUDED
#define BUFFER_H_INUCLUDED

#include "line.h"
#include "str.h"
#include "array.h"
#include "term.h"

#include <stddef.h>

typedef struct window window;

typedef enum {
        BP_NOP = 0,
        BP_INSERT,
        BP_INSERTNL,
        BP_MOV,
} buffer_proc;

typedef enum {
        BS_NORMAL = 0,
        BS_SEARCH,
        BS_SELECTION,
} buffer_state;

typedef struct {
        str          filename;    // file we are editing (if available)
        line_array   lns;         // lines
        size_t       cx;          // cursor x
        size_t       cy;          // cursor y
        size_t       al;          // active line
        size_t       wish_col;    // wished column
        size_t       hscrloff;    // horizontal scroll offset
        size_t       vscrloff;    // vertical scroll offset
        window      *parent;      // the parent window
        int          saved;       // has the document been saved?
        buffer_state state;       // our current state
        str          last_search; // last search query
        str          cpy;         // the copy buffer
        int          sy;          // the y position of selection mode
        int          sx;          // the x position of selection mode
} buffer;

DYN_ARRAY_TYPE(buffer *, bufferp_array);

buffer      *buffer_alloc(window *parent);
buffer      *buffer_from_file(str filename, window *parent);
buffer_proc  buffer_process(buffer *b, input_type ty, char ch);
void         buffer_dump(const buffer *b);
void         buffer_dump_xy(const buffer *b);
int          buffer_save(buffer *b);

#endif // BUFFER_H_INUCLUDED
