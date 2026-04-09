#include "ww.h"
#include "array.h"
#include "term.h"
#include "buffer.h"
#include "io.h"
#include "minibuffer.h"
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

static ssize_t
get_buffer_by_path(ww *ed, const char *path)
{
        for (size_t i = 0; i < ed->buffers.len; ++i) {
                if (!strcmp(ed->buffers.data[i]->path.chars, path)) {
                        return (ssize_t)i;
                }
        }

        return -1;
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

static void
ww_make_buffer_primary_by_path(ww *ed, const char *path)
{
        ssize_t idx;

        idx = -1;

        for (size_t i = 0; i < ed->buffers.len; ++i) {
                if (!strcmp(ed->buffers.data[i]->path.chars, path)) {
                        idx = (ssize_t)i;
                }
        }

        if (idx != -1) {
                ww_make_buffer_primary(ed, (size_t)idx);
        }
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


static void
find_file(ww *ed)
{
        cstr_ar files;
        char *chosen_file = NULL;
        str cwd = str_from(".");

 reload_dir:

        files = lsdir(str_cstr(&cwd));
        char *selected = minibuffer_completion_run(ed, "Find File", files);

        if (!selected)
                goto done;

        if (strcmp(selected, "..") == 0) {
                str_concat(&cwd, "/..");

                free(selected);
                array_free(files);
                goto reload_dir;
        }

        str fullpath = str_from_fmt("%s/%s",
                                    str_cstr(&cwd),
                                    selected);

        if (is_dir(str_cstr(&fullpath))) {
                str_destroy(&cwd);
                cwd = fullpath;

                free(selected);
                array_free(files);
                goto reload_dir;
        }

        chosen_file = strdup(get_realpath(fullpath.chars));

        str_destroy(&fullpath);
        free(selected);

 done:
        array_free(files);
        str_destroy(&cwd);

        if (!chosen_file) {
                buffer_draw(ed->monitors[ed->ab]);
                return;
        }

        if (!ww_buffer_exists_by_path(ed, chosen_file)) {
                ww_add_buffer(ed, buffer_from(str_from(get_basename(chosen_file)),
                                              str_from(chosen_file),
                                              (unsigned)glconf.term.w, (unsigned)glconf.term.h,
                                              0, 0,
                                              lines_from(load_file(chosen_file))));
        }

        free(chosen_file);

        ssize_t idx = get_buffer_by_path(ed, chosen_file);

        if (idx == -1)
                return;

        ww_make_buffer_primary(ed, (size_t)idx);

        buffer_draw(ed->buffers.data[ed->ab]);
}

void
ww_run(ww *ed)
{
        (void)ww_make_buffer_primary_by_path;

        ww_display_monitors(ed);
        gotoxy(0, 0);
        fflush(stdout);

        while (1) {
                buffer *b = ed->buffers.data[ed->ab];

                buffer_action act = buffer_process(b);

                if (act == BA_REDRAW) {
                        buffer_draw(b);
                } else if (act == BA_XY) {
                        buffer_drawxy(b);
                } else if (act == BA_REQ_EXIT) {
                        break;
                } else if (act == BA_REQ_FINDFILE) {
                        find_file(ed);
                }

                fflush(stdout);
        }
}
