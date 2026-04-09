#include "ww.h"
#include "array.h"
#include "term.h"
#include "buffer.h"
#include "glconf.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

ww
ww_create(void)
{
        return (ww) {
                .buffers  = array_empty(bufferp_ar),
                .monitors = {NULL, NULL, NULL, NULL},
                .ab       = 0,
        };
}

int
ww_buffer_exists_by_name(const ww   *ed,
                         const char *name)
{
        for (size_t i = 0; i < ed->buffers.len; ++i)
                if (!strcmp(ed->buffers.data[i]->name.chars, name))
                        return 1;
        return 0;
}

int
ww_buffer_exists_by_path(const ww   *ed,
                         const char *path)
{
        for (size_t i = 0; i < ed->buffers.len; ++i)
                if (!strcmp(ed->buffers.data[i]->path.chars, path))
                        return 1;
        return 0;
}

void
ww_add_buffer(ww *ed, buffer *b)
{
        if (ww_buffer_exists_by_name(ed, b->name.chars))
                str_overwrite(&b->name, b->path.chars);

        array_append(ed->buffers, NULL);

        for (size_t i = ed->buffers.len-1; i > 0; --i)
                ed->buffers.data[i] = ed->buffers.data[i-1];

        ed->buffers.data[0] = b;
}

void
ww_clear_monitors(ww *ed)
{
        memset(ed->monitors, 0, 4*sizeof(*ed->monitors));
}

void
ww_make_buffer_primary(ww *ed, size_t idx)
{
        assert(idx < ed->buffers.len);

        ww_clear_monitors(ed);
        ed->monitors[0] = ed->buffers.data[idx];
}

void
ww_display_monitors(ww *ed)
{
        for (size_t i = 0; i < 4; ++i) {
                if (ed->monitors[i]) {
                        ed->monitors[i]->size.w  = (unsigned)glconf.term.w;
                        ed->monitors[i]->size.h  = (unsigned)glconf.term.h;
                        ed->monitors[i]->size.ws = 0;
                        ed->monitors[i]->size.hs = 0;
                }
        }

        for (size_t i = 0; i < 4; ++i) {
                if (ed->monitors[i])
                        buffer_draw(ed->monitors[i]);
        }

        fflush(stdout);
}

void
ww_run(ww *ed)
{
        ww_display_monitors(ed);
        gotoxy(0, 0);
        fflush(stdout);

        while (1) {
                buffer *b = ed->buffers.data[ed->ab];

                buffer_action act = buffer_process(b);

                if (act == BA_REDRAW) {
                        buffer_draw(b);
                }
                else if (act == BA_XY) {
                        buffer_drawxy(b);
                }

                fflush(stdout);
        }
}
