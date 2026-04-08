#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#include "array.h"
#include "line.h"

typedef struct {
        struct {
                unsigned w;
                unsigned h;
                unsigned ws;
                unsigned hs;
        } size;
        line_ar lines;
} buffer;

ARRAY_DEFINE(buffer *, bufferp_ar);

#endif // BUFFER_H_INCLUDED
