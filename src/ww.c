#include "ww.h"
#include "array.h"
#include "term.h"
#include "buffer.h"
#include "io.h"
#include "minibuffer.h"
#include "flags.h"
#include "utils.h"
#include "tut.h"
#include "glconf.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <regex.h>

static volatile sig_atomic_t g_resize_flag = 0;

static void
resize_signal_handler(int sig)
{
        (void)sig;
        g_resize_flag = 1;
}

static void
handle_resize(ww *ed)
{
        if (!g_resize_flag)
                return;

        g_resize_flag = 0;

        size_t win_width;
        size_t win_height;

        if (!get_terminal_xy(&win_width, &win_height)) {
                perror("get_terminal_xy");
                exit(1);
        }

        glconf.term.w = win_width;
        glconf.term.h = win_height;

        ww_display_monitors(ed, BA_REDRAW);
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

static void
sort_buffers(ww *ed)
{
        if (ed->buffers.len <= 1)
                return;

        const buffer *curbuf = ed->monitors[ed->am];
        buffer *curbufi      = NULL;
        size_t idx           = 0;

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

        for (size_t i = idx; i > 0; --i)
                ed->buffers.data[i] = ed->buffers.data[i-1];

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
        char *selected = minibuffer_input(ed, "find-file", NULL, files);

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

        char *realpath = get_realpath(fullpath.chars);
        if (realpath)
                chosen_file = strdup(get_realpath(fullpath.chars));
        else
                chosen_file = strdup(fullpath.chars);

        str_destroy(&fullpath);
        free(selected);

 done:
        array_free(files);
        str_destroy(&cwd);

        if (!chosen_file) {
                buffer_draw(ed->monitors[ed->am]);
                return;
        }

        for (size_t i = 0; i < 4; ++i) {
                if (ed->monitors[i] && !strcmp(ed->monitors[i]->path.chars, chosen_file)) {
                        ed->am = (uint8_t)i;
                        buffer_draw(ed->monitors[ed->am]);
                        return;
                }
        }

        if (!ww_buffer_exists_by_path(ed, chosen_file)) {
                if (!file_exists(chosen_file))
                        create_file(chosen_file, 1);
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

        ed->monitors[ed->am] = ed->buffers.data[idx];
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
        cstr_ar names;
        char *selected;

        names = array_empty(cstr_ar);

        for (size_t i = 0; i < ed->buffers.len; ++i) {
                int found = 0;
                for (size_t j = 0; j < 4; ++j) {
                        if (ed->monitors[j] && !strcmp(ed->buffers.data[i]->name.chars,
                                                       ed->monitors[j]->name.chars)) {
                                found = 1;
                                break;
                        }
                }
                if (!found)
                        array_append(names, strdup(str_cstr(&ed->buffers.data[i]->name)));
        }

        selected = minibuffer_input(ed, "switch-buffer", NULL, names);

        if (!selected || strlen(selected) == 0)
                goto done;

        ed->monitors[ed->am] = get_buffer_by_name(ed, selected);

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
                if (!strcmp(ed->buffers.data[i]->name.chars, name))
                        idx = (ssize_t)i;
        }

        if (idx != -1)
                ww_make_buffer_primary(ed, (size_t)idx);
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
        //assert(idx < 4);

        ww_clear_monitors(ed);
        ed->monitors[0] = ed->buffers.data[idx];
        ed->am = 0;
        sort_buffers(ed);
}

static void
draw_monitor_based_on_action(ww            *ed,
                             buffer_action  ba,
                             size_t         idx)
{
        if (!ed->monitors[idx])
                return;

        if (ba == BA_REDRAW
            || ba == BA_REQ_SPLITVER
            || ba == BA_REQ_JMPBUF
            || ba == BA_REQ_SWITCHBUFFER
            || ba == BA_REQ_FINDFILE
            || ba == BA_REQ_MAXIMIZEMON
            || ba == BA_REQ_COMPILE
            || ba == BA_REQ_RECOMPILE
            || ba == BA_REQ_CLOSE_BUILTIN
            || ba == BA_REQ_SPLITHOR
            || ba == BA_REQ_KILLBUF
            || ba == BA_REQ_SWITCHCOMPL
            || ba == BA_REQ_ERRJMP)
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
        /*if (ed->buffers.len <= 1)
                return;*/

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

        if (!b) {
                b = ww_helpbuf_alloc((unsigned)glconf.term.w, (unsigned)glconf.term.w, 0, 0, ed);
                array_append(ed->buffers, b);
        }

        ed->monitors[1] = b;
        //ed->monitors[1] = ed->monitors[ed->ab];
        ed->am = 1;
}

static void
split_horizontal(ww *ed)
{
        /*if (ed->buffers.len <= 1)
                return;*/

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

        if (!b) {
                b = ww_helpbuf_alloc((unsigned)glconf.term.w, (unsigned)glconf.term.w, 0, 0, ed);
                array_append(ed->buffers, b);
        }

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

typedef void (*output_callback)(const char *chunk, size_t len, void *userdata);

static void
append_to_buffer_callback(const char *chunk,
                          size_t      len,
                          void       *userdata)
{
        buffer *b = (buffer *)userdata;
        char tmp[len + 1];
        memcpy(tmp, chunk, len);
        tmp[len] = '\0';

        linep_ar new_lines = lines_from(tmp);
        for (size_t i = 0; i < new_lines.len; ++i) {
                array_append(b->lines, new_lines.data[i]);
                ++b->al;
                ++b->cy;
        }
        array_free(new_lines);

        buffer_adjust_scroll(b);
        buffer_draw(b);
}

static void
capture_command_output_stream(str             *input,
                              output_callback  cb,
                              void            *userdata)
{
        int   pipefd[2];
        pid_t pid;

        if (pipe(pipefd) == -1) {
                perror("pipe");
                return;
        }

        pid = fork();
        if (pid == -1) {
                perror("fork");
                close(pipefd[0]);
                close(pipefd[1]);
                return;
        }

        if (pid == 0) {
                // child
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                dup2(pipefd[1], STDERR_FILENO);
                close(pipefd[1]);
                execl("/bin/sh", "sh", "-c", input->chars, NULL);
                _exit(127); // exec failed
        }

        // parent
        close(pipefd[1]);

        char buf[4096];
        ssize_t n;
        while ((n = read(pipefd[0], buf, sizeof(buf))) > 0)
                cb(buf, (size_t)n, userdata); // sending chunk to buffer

        close(pipefd[0]);
        waitpid(pid, NULL, 0);
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
                array_clear(b->lines);
        }

        if (!exists)
                ww_add_buffer(ed, b);

        ed->monitors[ed->am] = ed->buffers.data[get_buffer_by_path(ed, BUFFER_BUILTIN_COMPILE)];

        ed->monitors[ed->am]->cx = 0;
        ed->monitors[ed->am]->al = 0;
        ed->monitors[ed->am]->cy = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
        char buf[1024] = {0};
        sprintf(buf, COMPILATION_HEADER, str_cstr(&input));
        linep_ar header = lines_from(buf);
        for (int i = (int)header.len-1; i >= 0; --i)
                array_insert_at(ed->monitors[ed->am]->lines, 0, header.data[i]);
#pragma GCC diagnostic pop
        buffer_adjust_scroll(ed->monitors[ed->am]);

        buffer_draw(ed->monitors[ed->am]);
        capture_command_output_stream(&input, append_to_buffer_callback, ed->monitors[ed->am]);
        array_append(ed->monitors[ed->am]->lines, line_from(str_from("\n")));
        array_append(ed->monitors[ed->am]->lines, line_from(str_from("[ Done ] ")));
        ed->monitors[ed->am]->al = ed->monitors[ed->am]->lines.len-1;
        ed->monitors[ed->am]->cy = (unsigned)ed->monitors[ed->am]->lines.len-1;
        buffer_adjust_scroll(ed->monitors[ed->am]);

        str_destroy(&input);
        buffer_draw(ed->monitors[ed->am]);

#undef COMPILATION_HEADER
}

static void
compile(ww *ed)
{
        char *input_raw = minibuffer_input(ed, "compile", glconf.runtime.compile,
                                           array_empty(cstr_ar));

        if (!input_raw || strlen(input_raw) == 0)
                return;

        if (strcmp(glconf.runtime.compile, input_raw)) {
                free(glconf.runtime.compile);
                glconf.runtime.compile = strdup(input_raw);
        }

        do_compilation(ed);
        sort_buffers(ed);
}

static void
toggle_spacemode(void)
{
        glconf.flags ^= FK_TABMODE;
}

static void
set_spaceamt(ww *ed)
{
        cstr_ar items;

        items = array_empty(cstr_ar);

        array_append(items, "1");
        array_append(items, "2");
        array_append(items, "4");
        array_append(items, "8");

        char *input = minibuffer_input(ed, "space-amt", NULL, items);

        if (!input || strlen(input) == 0)
                goto cleanup;
        if (!cstr_isdigit(input))
                goto cleanup;

        glconf.runtime.space_amt = atoi(input);

cleanup:
        array_free(items);
}

static void
tutorial(ww *ed)
{
        char *input;
        cstr_ar chapters;
        buffer *b;

        chapters = array_empty(cstr_ar);
        array_append(chapters, TUT_CH1_NAME);
        array_append(chapters, TUT_CH2_NAME);

        input = minibuffer_input(ed, "tutorial", NULL, chapters);

        if (!input || strlen(input) == 0)
                goto done;

        ssize_t idx = get_buffer_by_path(ed, input);

        if (idx == -1) {
                b = tut_alloc(ed, input);
                if (!b) goto done;
                ww_add_buffer(ed, b);
        }
        else
                b = ed->buffers.data[(size_t)idx];

        ed->monitors[ed->am] = b;

done:
        array_free(chapters);
}

static void
help(ww *ed)
{
        ssize_t idx;
        buffer *b;

        if ((idx = get_buffer_by_path(ed, BUFFER_BUILTIN_HELP)) == -1) {
                b = ww_helpbuf_alloc((unsigned)glconf.term.w, (unsigned)glconf.term.w, 0, 0, ed);
                array_append(ed->buffers, b);
        }
        else
                b = ed->buffers.data[(size_t)idx];

        ed->monitors[ed->am] = b;
}

static void
metax(ww *ed)
{
        char *inp;

        static char *cmds_raw[] = WW_CMD_CPL;
        cstr_ar            cmds = array_empty(cstr_ar);

        for (size_t i = 0; i < sizeof(cmds_raw)/sizeof(*cmds_raw); ++i)
                array_append(cmds, cmds_raw[i]);

        if (!(inp = minibuffer_input(ed, "M-x", NULL, cmds)))
                return;

        if (!strcmp(inp, WW_CMD_SAVE))
                buffer_save(ed->monitors[ed->am]);
        else if (!strcmp(inp, WW_CMD_FIND_FILE))
                find_file(ed);
        else if (!strcmp(inp, WW_CMD_COMPILE))
                compile(ed);
        else if (!strcmp(inp, WW_CMD_TOGGLE_SPACEMODE))
                toggle_spacemode();
        else if (!strcmp(inp, WW_CMD_SPACEAMT))
                set_spaceamt(ed);
        else if (!strcmp(inp, WW_CMD_TUT))
                tutorial(ed);
        else if (!strcmp(inp, WW_CMD_HELP))
                help(ed);
        else if (!strcmp(inp, WW_CMD_QUIT))
                exit(0);

        free(inp);
        array_free(cmds);
}

static void
close_builtin(ww *ed)
{
        ed->monitors[ed->am] = ed->buffers.data[1];
        sort_buffers(ed);
}

static buffer *
get_random_nonopen_buffer(ww *ed)
{
        for (size_t i = 0; i < ed->buffers.len; ++i) {
                const char *name  = ed->buffers.data[i]->name.chars;
                int         found = 0;

                for (int j = 0; j < 4; ++j) {
                        if (ed->monitors[j] && !strcmp(name, ed->monitors[j]->name.chars)) {
                                found = 1;
                                break;
                        }
                }

                if (!found)
                        return ed->buffers.data[i];
        }

        return NULL;
}

static void
kill_current_buffer(ww *ed)
{
        size_t  idx;
        buffer *b;

        idx = (size_t)get_buffer_by_path(ed, ed->monitors[ed->am]->path.chars);

        array_rm_at(ed->buffers, idx);

        if (ed->buffers.len == 0) {
                buffer_free(ed->monitors[ed->am]);
                ed->monitors[ed->am] = NULL;
                return;
        }

        if (!(b = get_random_nonopen_buffer(ed))) {
                idx = (size_t)get_buffer_by_path(ed, ed->monitors[0]->path.chars);
                buffer_free(ed->monitors[ed->am]);
                ed->monitors[ed->am] = NULL;
                ww_make_buffer_primary(ed, idx);
                return;
        }

        buffer_free(ed->monitors[ed->am]);
        ed->monitors[ed->am] = b;
}

static void
switch_to_compilation_buffer(ww *ed)
{
        for (size_t i = 0; i < ed->buffers.len; ++i) {
                if (!strcmp(str_cstr(&ed->buffers.data[i]->name),
                            BUFFER_BUILTIN_COMPILE)) {
                        ed->monitors[ed->am] = ed->buffers.data[i];
                        sort_buffers(ed);
                        return;
                }
        }
}

static void
try_jump_to_error(ww *ed)
{
        const line *ln       = NULL;
        char       *filename = NULL;
        int         row      = -1;
        int         col      = -1;
        buffer     *ab       = NULL;

        ab = ed->monitors[ed->am];
        ln = ab->lines.data[ab->al];

        regex_t regex;
        regmatch_t matches[4]; // full match + 3 capture groups

        const char *pattern = "([^[:space:]:]+):([0-9]+):([0-9]+):";

        if (regcomp(&regex, pattern, REG_EXTENDED)) {
                fprintf(stderr, "Could not compile regex\n");
                return;
        }

        if (regexec(&regex, ln->txt.chars, 4, matches, 0) == 0) {
                int fname_len = matches[1].rm_eo - matches[1].rm_so;

                if (!(filename = malloc((size_t)fname_len + 1))) {
                        regfree(&regex);
                        return;
                }

                memcpy(filename,
                       ln->txt.chars + matches[1].rm_so,
                       (size_t)fname_len);
                filename[fname_len] = '\0';

                row = atoi(ln->txt.chars + matches[2].rm_so);
                col = atoi(ln->txt.chars + matches[3].rm_so);
        }

        regfree(&regex);

        if (!filename) {
                buffer_draw(ab);
                return;
        }

        if (row == -1 || col == -1) {
                free(filename);
                buffer_draw(ab);
                return;
        }

        buffer *b = NULL;
        char *real = get_realpath(filename);
        assert(real);

        if (!ww_buffer_exists_by_path(ed, real)) {
                b = buffer_from(str_from(filename),
                                str_from(real),
                                (unsigned)glconf.term.w, (unsigned)glconf.term.h,
                                0, 0,
                                lines_from(load_file(real)), ed);
                ww_add_buffer(ed, b);
        }
        else
                b = ed->buffers.data[(size_t)get_buffer_by_path(ed, real)];

        ed->monitors[ed->am] = b;
        buffer_jump_to_verts(ed->monitors[ed->am], (size_t)col-1, (size_t)row-1);

        free(filename);
        buffer_draw(ed->monitors[ed->am]);

        sort_buffers(ed);
}

void
ww_run(ww *ed)
{
        (void)ww_make_buffer_primary_by_path;
        (void)make_command;

        signal(SIGWINCH, resize_signal_handler);

        ww_display_monitors(ed, BA_REDRAW);
        gotoxy(0, 0);
        fflush(stdout);

        while (ed->monitors[0]) {
                assert(ed->am < 4);

                handle_resize(ed);

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
                else if (act == BA_REQ_KILLBUF)       kill_current_buffer(ed);
                else if (act == BA_REQ_SWITCHCOMPL)   switch_to_compilation_buffer(ed);
                else if (act == BA_REQ_ERRJMP)        try_jump_to_error(ed);

                ww_display_monitors(ed, act);
        }
}
