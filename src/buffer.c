#include "buffer.h"
#include "io.h"

#include <assert.h>
#include <stdlib.h>

buffer *
buffer_alloc(void)
{
        buffer *b = (buffer *)malloc(sizeof(buffer));

        b->filename = str_create();
        b->lns      = dyn_array_empty(line_array);
        b->cx       = 0;
        b->cy       = 0;
        b->al       = NULL;

        return b;
}

buffer *
buffer_from_file(str filename)
{
        buffer *b;

        b = buffer_alloc();
        str_destroy(&b->filename);
        b->filename = filename;

        if (file_exists(str_cstr(&filename))) {
                char       *file_data;

                file_data = load_file(str_cstr(&filename));
                b->lns    = lines_of_cstr(file_data);
                b->al     = &b->lns.data[0];
        }

        return b;
}

buffer_proc
buffer_process(buffer     *b,
               input_type  ty,
               char        ch)
{
        assert(0);
}

void
buffer_dump(const buffer *b)
{
        assert(0);
}
