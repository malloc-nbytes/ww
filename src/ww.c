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
#include <unistd.h>
#include <sys/wait.h>

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
sort_buffers(ww *ed)
{
        if (ed->buffers.len <= 1)
                return;

        const buffer *curbuf = ed->monitors[ed->am];
        buffer *curbufi = NULL;
        size_t idx = 0;

        for (size_t i = 0; i < ed->buffers.len; ++i) {
                if (!strcmp(ed->buffers.data[i]->name.chars, curbuf->name.chars)) {
                        curbufi = ed->buffers.data[i];
                        idx = i;
                        break;
                }
        }

        if (!curbufi) {
                assert(0);
                return;
        }

        ed->buffers.data[idx] = ed->buffers.data[0];
        ed->buffers.data[0]   = curbufi;
}

static void
find_file(ww *ed)
{
        cstr_ar  files;
        char    *chosen_file = NULL;
        str      cwd         = str_from(".");

 reload_dir:

        files          = lsdir(str_cstr(&cwd));
        char *selected = minibuffer_input(ed, "find-file", files);

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
                buffer_draw(ed->monitors[ed->am]);
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

        buffer_draw(ed->buffers.data[ed->am]);

        sort_buffers(ed);
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

        char *selected = minibuffer_input(ed, "switch-buffer", names);

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
                ed->monitors[ed->am] = get_buffer_by_name(ed, selected);
        else
                ww_make_buffer_primary_by_name(ed, selected);

        sort_buffers(ed);

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
                .am       = 0,
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
        ed->am = 0;
}

static void
draw_monitor_based_on_action(ww            *ed,
                             buffer_action  ba,
                             size_t         idx)
{
        if (ba == BA_REDRAW
            || ba == BA_REQ_SPLITVER
            || ba == BA_REQ_JMPBUF
            || ba == BA_REQ_SWITCHBUFFER
            || ba == BA_REQ_FINDFILE
            || ba == BA_REQ_MAXIMIZEMON
            || ba == BA_REQ_COMPILE
            || ba == BA_REQ_RECOMPILE
            || ba == BA_REQ_CLOSE_BUILTIN
            || ba == BA_REQ_SPLITHOR)
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

        if (ed->monitors[2]) {
                ed->monitors[2]->size.hs = (unsigned)glconf.term.h/2;
                ed->monitors[0]->size.h /= 2;
                if (ed->monitors[1])
                        ed->monitors[1]->size.h /= 2;
        }

        if (ba == BA_NOP)
                return;

        for (size_t i = 0; i < 4; ++i) {
                if (i != ed->am && ed->monitors[i])
                        draw_monitor_based_on_action(ed, ba, i);
        }

        // Draw active monitor lastly to not re-draw.
        draw_monitor_based_on_action(ed, ba, ed->am);

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
        ed->am = 1;
}

static void
split_horizontal(ww *ed)
{
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

        ed->monitors[2] = b;
        ed->am          = 2;
}

static void
jump_buffer(ww *ed)
{
        do {
                ed->am = (uint8_t)(ed->am + 1)
                        % (sizeof(ed->monitors)/sizeof(*ed->monitors));
        } while (!ed->monitors[ed->am]);
}

static char **
make_command(str *s)
{
        cstr_ar ar  = array_empty(cstr_ar);
        char_ar buf = array_empty(char_ar);

        for (size_t i = 0; i < s->len; ++i) {
                char ch = str_at(s, i);
                if (ch == ' ') {
                        if (buf.len > 0) {
                                array_append(buf, 0);
                                array_append(ar, strdup(buf.data));
                                array_clear(buf);
                        }
                } else {
                        array_append(buf, ch);
                }
        }

        if (buf.len > 0) {
                array_append(buf, 0);
                array_append(ar, strdup(buf.data));
        }

        array_append(ar, NULL);
        array_free(buf);

        return ar.data;
}

static char *
capture_command_output(str *input)
{
        int     pipefd[2];
        pid_t   pid;
        char   *output      = NULL;
        size_t  output_size = 0;
        size_t  output_cap  = 0;
        char    buf[4096];

        if (pipe(pipefd) == -1) {
                perror("pipe");
                return NULL;
        }

        pid = fork();
        if (pid == -1) {
                perror("fork");
                close(pipefd[0]);
                close(pipefd[1]);
                return NULL;
        }

        if (pid == 0) {
                // Child
                close(pipefd[0]);

                if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                        perror("dup2 stdout");
                        _exit(1);
                }

                if (dup2(pipefd[1], STDERR_FILENO) == -1) {
                        perror("dup2 stderr");
                        _exit(1);
                }

                close(pipefd[1]);

                char **args = make_command(input);
                if (!args || !*args)
                        _exit(127);

                execvp(args[0], args);
                perror("execvp");
                fflush(stdout);
                _exit(127);
        }

        // Parent
        close(pipefd[1]);

        // Read loop
        ssize_t n;
        while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
                buf[n] = '\0';

                size_t needed = output_size + (size_t)n + 1;
                if (needed > output_cap) {
                        output_cap = needed * 2 < 8192 ? 8192 : needed * 2;
                        char *new_out = realloc(output, output_cap);
                        if (!new_out) {
                                perror("realloc");
                                free(output);
                                close(pipefd[0]);
                                waitpid(pid, NULL, 0);
                                return NULL;
                        }
                        output = new_out;
                }

                memcpy(output + output_size, buf, (size_t)n);
                output_size += (size_t)n;
                output[output_size] = '\0';
        }

        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);

        return output;
}

static void
do_compilation(ww *ed)
{
#define COMPILATION_HEADER "*** Compilation [ %s ] [ (q)uit, a(g)ain, M-<tab>:switch-here ] ***\n\n"

        if (!glconf.runtime.compile)
                return;

        str     input  = str_from(glconf.runtime.compile);
        buffer *b      = NULL;
        int     exists = 0;

        b = get_buffer_by_name(ed, BUFFER_BUILTIN_COMPILE);

        if (!b) {
                b = buffer_from(str_from(BUFFER_BUILTIN_COMPILE),
                                str_from(BUFFER_BUILTIN_COMPILE),
                                (unsigned)glconf.term.w, (unsigned)glconf.term.h,
                                0, 0,
                                array_empty(linep_ar), ed);
                buffer_make_readonly(b);
                buffer_make_builtin(b);
        } else {
                exists = 1;
                for (size_t i = 0; i < b->lines.len; ++i)
                        line_free(b->lines.data[i]);
                array_free(b->lines);
        }

        if (!exists)
                ww_add_buffer(ed, b);
        ww_make_buffer_primary_by_name(ed, BUFFER_BUILTIN_COMPILE);
        buffer_draw(ed->monitors[ed->am]);

        char *output = capture_command_output(&input);
        ed->monitors[ed->am]->lines = lines_from(output);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
        char buf[1024] = {0};
        sprintf(buf, COMPILATION_HEADER, str_cstr(&input));
        linep_ar header = lines_from(buf);
        for (int i = (int)header.len-1; i >= 0; --i)
                array_insert_at(ed->monitors[ed->am]->lines, 0, header.data[i]);
#pragma GCC diagnostic pop

        array_append(ed->monitors[ed->am]->lines, line_from(str_from("\n")));
        array_append(ed->monitors[ed->am]->lines, line_from(str_from("[ Done ] ")));
        ed->monitors[ed->am]->cx = 0;
        ed->monitors[ed->am]->al = 0;
        ed->monitors[ed->am]->cy = 0;
        buffer_adjust_scroll(ed->monitors[ed->am]);

        str_destroy(&input);
        buffer_draw(ed->monitors[ed->am]);

#undef COMPILATION_HEADER
}

static void
compile(ww *ed)
{
        char *input_raw = minibuffer_input(ed, "compile", array_empty(cstr_ar));

        if (!input_raw)
                return;

        glconf.runtime.compile = strdup(input_raw);

        do_compilation(ed);
}

static void
toggle_spacemode(void)
{
        glconf.runtime.spacemode = !glconf.runtime.spacemode;
}

static void
metax(ww *ed)
{
        char *inp;

        static char *cmds_raw[] = WW_CMD_CPL;
        cstr_ar            cmds = array_empty(cstr_ar);

        for (size_t i = 0; i < sizeof(cmds_raw)/sizeof(*cmds_raw); ++i)
                array_append(cmds, cmds_raw[i]);

        if (!(inp = minibuffer_input(ed, "M-x", cmds)))
                return;

        if (!strcmp(inp, WW_CMD_SAVE))
                buffer_save(ed->monitors[ed->am]);
        else if (!strcmp(inp, WW_CMD_FIND_FILE))
                find_file(ed);
        else if (!strcmp(inp, WW_CMD_COMPILE))
                compile(ed);
        else if (!strcmp(inp, WW_CMD_TOGGLE_SPACEMODE))
                toggle_spacemode();

        free(inp);
        array_free(cmds);
}

static void
close_builtin(ww *ed)
{
        ed->monitors[ed->am] = ed->buffers.data[0];
}

void
ww_run(ww *ed)
{
        (void)ww_make_buffer_primary_by_path;
        (void)do_compilation;

        ww_display_monitors(ed, BA_REDRAW);
        gotoxy(0, 0);
        fflush(stdout);

        while (1) {
                assert(ed->am < 4);

                buffer *b = ed->monitors[ed->am];

                buffer_action act = buffer_process(b);

                if      (act == BA_REQ_EXIT)         break;
                else if (act == BA_REQ_FINDFILE)     find_file(ed);
                else if (act == BA_REQ_SWITCHBUFFER) ww_switch_buffer(ed);
                else if (act == BA_REQ_MAXIMIZEMON)
                        ww_make_buffer_primary_by_name(ed, ed->monitors[ed->am]->name.chars);
                else if (act == BA_REQ_METAX) {
                        metax(ed);
                        buffer_draw(ed->monitors[ed->am]);
                }
                else if (act == BA_REQ_SPLITVER)      split_vertical(ed);
                else if (act == BA_REQ_JMPBUF)        jump_buffer(ed);
                else if (act == BA_REQ_COMPILE)       compile(ed);
                else if (act == BA_REQ_RECOMPILE)     do_compilation(ed);
                else if (act == BA_REQ_CLOSE_BUILTIN) close_builtin(ed);
                else if (act == BA_REQ_SPLITHOR)      split_horizontal(ed);

                ww_display_monitors(ed, act);
        }
}
