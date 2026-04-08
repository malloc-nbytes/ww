#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#include "array.h"
#include "line.h"
#include "str.h"

typedef struct {
        str name;
        str path;
        struct {
                unsigned w;
                unsigned h;
                unsigned ws;
                unsigned hs;
        } size;
        line_ar lines;
        size_t  cx;
        size_t  cy;
        size_t  al;
        size_t  voff;
        size_t  hoff;
} buffer;

ARRAY_DEFINE(buffer *, bufferp_ar);

buffer *buffer_from(str      name,
                    str      path,
                    unsigned w,
                    unsigned h,
                    unsigned ws,
                    unsigned hs,
                    line_ar  lns);

#endif // BUFFER_H_INCLUDED
