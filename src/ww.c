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

static void
find_file(ww *ed)
{
        cstr_ar  files;
        char    *chosen_file = NULL;
        str      cwd         = str_from(".");

 reload_dir:

        files          = lsdir(str_cstr(&cwd));
        char *selected = minibuffer_completion_run(ed, "find-file", files);

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
                                              lines_from(load_file(chosen_file)), ed));
        }

        ssize_t idx = get_buffer_by_path(ed, chosen_file);

        free(chosen_file);

        if (idx == -1)
                return;

        ww_make_buffer_primary(ed, (size_t)idx);

        buffer_draw(ed->buffers.data[ed->ab]);
}

static buffer *
get_buffer_by_name(ww         *ed,
                   const char *name)
{
        for (size_t i = 0; i < ed->buffers.len; ++i) {
                if (!strcmp(ed->buffers.data[i]->name.chars, name))
                        return ed->buffers.data[i];
        }
        return NULL;
}

void
ww_switch_buffer(ww *ed)
{
        cstr_ar names = array_empty(cstr_ar);

        for (size_t i = 0; i < ed->buffers.len; ++i)
                array_append(names, strdup(str_cstr(&ed->buffers.data[i]->name)));

        char *selected = minibuffer_completion_run(ed, "switch-buffer", names);

        if (!selected)
                goto done;

        int open = 0;
        for (size_t i = 0; i < 4; ++i) {
                if (!ed->monitors[i])
                        continue;
                if (!strcmp(ed->monitors[i]->name.chars, selected)) {
                        open = 1;
                        break;
                }
        }

        if (!open)
                ed->monitors[ed->ab] = get_buffer_by_name(ed, selected);
        else
                ww_make_buffer_primary_by_name(ed, selected);

 done:
        free(selected);
        for (size_t i = 0; i < names.len; ++i)
                free(names.data[i]);
        array_free(names);
}

void
ww_make_buffer_primary_by_name(ww *ed, const char *name)
{
        ssize_t idx;

        idx = -1;

        for (size_t i = 0; i < ed->buffers.len; ++i) {
                if (!strcmp(ed->buffers.data[i]->name.chars, name)) {
                        idx = (ssize_t)i;
                }
        }

        if (idx != -1) {
                ww_make_buffer_primary(ed, (size_t)idx);
        }
}

void
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

        //array_append(ed->buffers, NULL);
        array_append(ed->buffers, b);

        //for (size_t i = ed->buffers.len-1; i > 0; --i)
        //        ed->buffers.data[i] = ed->buffers.data[i-1];

        //ed->buffers.data[0] = b;
}

void
ww_clear_monitors(ww *ed)
{
        ed->monitors[0] = NULL;
        ed->monitors[1] = NULL;
        ed->monitors[2] = NULL;
        ed->monitors[3] = NULL;
}

void
ww_make_buffer_primary(ww *ed, size_t idx)
{
        assert(idx < 4);

        ww_clear_monitors(ed);
        ed->monitors[0] = ed->buffers.data[idx];
        ed->ab = 0;
}

static void
draw_monitor_based_on_action(ww            *ed,
                             buffer_action  ba,
                             size_t         idx)
{
        if (ba == BA_REDRAW
            || ba == BA_REQ_SPLITHOR
            || ba == BA_REQ_JMPBUF
            || ba == BA_REQ_SWITCHBUFFER
            || ba == BA_REQ_FINDFILE)
                buffer_draw(ed->monitors[idx]);
        else if (ba == BA_XY)
                buffer_drawxy(ed->monitors[idx]);
}

void
ww_display_monitors(ww *ed, buffer_action ba)
{
        for (size_t i = 0; i < 4; ++i) {
                if (ed->monitors[i]) {
                        ed->monitors[i]->size.w  = (unsigned)glconf.term.w;
                        ed->monitors[i]->size.h  = (unsigned)glconf.term.h;
                        ed->monitors[i]->size.ws = 0;
                        ed->monitors[i]->size.hs = 0;
                }
        }

        if (ed->monitors[1]) {
                ed->monitors[0]->size.w  /= 2;
                ed->monitors[1]->size.w  /= 2;
                ed->monitors[1]->size.ws  = (unsigned)glconf.term.w/2;
        }

        if (ba == BA_NOP)
                return;

        for (size_t i = 0; i < 4; ++i) {
                if (i != ed->ab && ed->monitors[i])
                        draw_monitor_based_on_action(ed, ba, i);
        }

        // Draw active monitor lastly to not re-draw.
        draw_monitor_based_on_action(ed, ba, ed->ab);

        fflush(stdout);
}

static void
split_vertical(ww *ed)
{
        // TODO: check for monitor[2]
        if (ed->buffers.len <= 1)
                return;

        buffer *b = NULL;

        for (size_t i = 0; i < ed->buffers.len; ++i) {
                b = ed->buffers.data[i];

                for (size_t j = 0; j < 4; ++j) {
                        if (!ed->monitors[j])
                                continue;
                        if (!strcmp(ed->monitors[j]->name.chars,
                                    ed->buffers.data[i]->name.chars)) {
                                b = NULL;
                                break;
                        }
                }

                if (b)
                        break;
        }

        if (!b)
                return;

        ed->monitors[1] = b;
        //ed->monitors[1] = ed->monitors[ed->ab];
        ed->ab = 1;
}

static void
jump_buffer(ww *ed)
{
        do {
                ed->ab = (uint8_t)(ed->ab+1) % 4;
        } while (!ed->monitors[ed->ab]);
}

void
ww_run(ww *ed)
{
        (void)ww_make_buffer_primary_by_path;

        ww_display_monitors(ed, BA_REDRAW);
        gotoxy(0, 0);
        fflush(stdout);

        while (1) {
                assert(ed->ab < 4);

                buffer *b = ed->monitors[ed->ab];

                buffer_action act = buffer_process(b);

                if      (act == BA_REQ_EXIT)         break;
                else if (act == BA_REQ_FINDFILE)     find_file(ed);
                else if (act == BA_REQ_SWITCHBUFFER) ww_switch_buffer(ed);
                else if (act == BA_REQ_SPLITHOR)     split_vertical(ed);
                else if (act == BA_REQ_JMPBUF)       jump_buffer(ed);

                ww_display_monitors(ed, act);
        }
}
