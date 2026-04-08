#include "buffer.h"
#include "mem.h"

#include <assert.h>

buffer *
buffer_from(str      name,
            str      path,
            unsigned w,
            unsigned h,
            unsigned ws,
            unsigned hs,
            line_ar  lns)
{
        buffer *b;

        b          = (buffer *)alloc(sizeof(buffer));
        b->name    = name;
        b->path    = path;
        b->size.w  = w;
        b->size.h  = h;
        b->size.ws = ws;
        b->size.hs = hs;
        b->lines   = lns;

        return b;
}

void
buffer_draw(const buffer *b)
{
        assert(b && 0);
}
