#ifndef WW_H_INCLUDED
#define WW_H_INCLUDED

#include "buffer.h"

typedef struct {
        bufferp_ar buffers;
        buffer *monitors[4];
} ww;

#endif // WW_H_INCLUDED
