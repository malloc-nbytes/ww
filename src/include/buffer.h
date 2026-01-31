#ifndef BUFFER_H_INUCLUDED
#define BUFFER_H_INUCLUDED

#include "line.h"
#include "str.h"
#include "array.h"
#include "term.h"

#include <stddef.h>

typedef enum {
        BP_NOP,
        BP_UPDATE = 0,
        BP_MOV,
} buffer_proc;

typedef struct {
        str          filename;
        line_array   lns;
        size_t       cx;
        size_t       cy;
        line       **al;
} buffer;

DYN_ARRAY_TYPE(buffer *, bufferp_array);

buffer      *buffer_alloc(void);
buffer      *buffer_from_file(str filename);
buffer_proc  buffer_process(buffer *b, input_type ty, char ch);
void         buffer_dump(const buffer *b);

#endif // BUFFER_H_INUCLUDED
