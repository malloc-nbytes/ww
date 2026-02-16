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

typedef struct {
        str          filename;
        line_array   lns;      // lines
        size_t       cx;       // cursor x
        size_t       cy;       // cursor y
        size_t       al;       // active line
        size_t       wish_col; // wished column
        size_t       hscrloff; // horizontal scroll offset
        size_t       vscrloff; // vertical scroll offset
        window      *parent;
        int          saved;
        str          cpy;
} buffer;

DYN_ARRAY_TYPE(buffer *, bufferp_array);

buffer      *buffer_alloc(window *parent);
buffer      *buffer_from_file(str filename, window *parent);
buffer_proc  buffer_process(buffer *b, input_type ty, char ch);
void         buffer_dump(const buffer *b);
void         buffer_dump_xy(const buffer *b);
int          buffer_save(buffer *b);

#endif // BUFFER_H_INUCLUDED
