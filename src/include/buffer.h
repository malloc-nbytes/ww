#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#include "array.h"
#include "line.h"
#include "str.h"

typedef enum {
        BA_NOP = 0,
        BA_REDRAW,
        BA_XY,
} buffer_action;

typedef enum {
        BS_NORMAL,
        BS_SEARCH,
        BS_SELECTION,
} buffer_state;

typedef struct {
        str name; // name of buffer, can be same as path
                  // if buffer by the same name exists
        str path; // path to the file we are editing
        struct {
                unsigned w;  // width
                unsigned h;  // height
                unsigned ws; // width start
                unsigned hs; // height start
        } size;
        linep_ar     lines; // lines in the buffer
        unsigned     cx;    // cursor x
        unsigned     cy;    // cursor y (visual)
        size_t       al;    // active line
        size_t       voff;  // vertical scroll offset
        size_t       hoff;  // horizontal scroll offset
        buffer_state state; // current state
        int          saved; // is the buffer saved
        unsigned     sx;    // buffer selection x
        unsigned     sy;    // buffer selection y
} buffer;

ARRAY_DEFINE(buffer *, bufferp_ar);

buffer *buffer_from(str      name,
                    str      path,
                    unsigned w,
                    unsigned h,
                    unsigned ws,
                    unsigned hs,
                    linep_ar lns);

void buffer_draw(const buffer *b);
void buffer_drawxy(const buffer *b);
buffer_action buffer_process(buffer *b);

#endif // BUFFER_H_INCLUDED
