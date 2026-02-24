#include "window.h"
#include "term.h"
#include "io.h"
#include "trie.h"
#include "str.h"
#include "colors.h"
#include "fuzzy.h"
#include "glconf.h"
#include "flags.h"
#include "utils.h"
#include "controls-buffer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <regex.h>

#define MAX_COMPLETIONS  200
#define DISPLAY_COUNT     6
#define MAX_COMPLETIONS_REQUEST  200
#define MAX_VERTICAL_LINES        8
#define FILE_SELECTION_PADDING 10

typedef struct {
        str input;
        size_t selected_idx;
        size_t offset;
} completion_state;

static void
completion_draw(window *win,
                const char *label,
                completion_state *st,
                char **completions,
                size_t total_matches)
{
        /* Keep indices sane */
        if (total_matches == 0) {
                st->selected_idx = 0;
                st->offset = 0;
        } else {
                if (st->selected_idx >= total_matches)
                        st->selected_idx = total_matches - 1;

                if (st->selected_idx < st->offset)
                        st->offset = st->selected_idx;
                else if (st->selected_idx >=
                         st->offset + MAX_VERTICAL_LINES)
                        st->offset = st->selected_idx - MAX_VERTICAL_LINES + 1;

                size_t max_offset =
                        total_matches > MAX_VERTICAL_LINES
                        ? total_matches - MAX_VERTICAL_LINES
                        : 0;

                if (st->offset > max_offset)
                        st->offset = max_offset;
        }

        /* Prompt line */
        gotoxy(0, win->h);
        clear_line(0, win->h);

        printf("%s [ %s ]",
               label,
               str_cstr(&st->input));

        if (total_matches > 0)
                printf(" (%zu/%zu)",
                       st->selected_idx + 1,
                       total_matches);

        if (total_matches >= MAX_COMPLETIONS_REQUEST)
                printf(" [more...]");

        /* Completion line */
        gotoxy(0, win->h - 1);
        clear_line(0, win->h - 1);

        if (total_matches == 0) {
                printf("(no matches)");
        } else {
                size_t cursor_x = 0;
                size_t items_shown = 0;
                const size_t sep_len = strlen(" | ");

                for (size_t i = st->offset; i < total_matches; ++i) {
                        const char *name =
                                completions[i]
                                ? completions[i]
                                : "?";

                        size_t name_len = strlen(name);

                        size_t would_be = cursor_x;
                        if (items_shown > 0)
                                would_be += sep_len;
                        would_be += name_len;

                        if (would_be > win->w - FILE_SELECTION_PADDING) {

                                size_t remaining = win->w - cursor_x;

                                if (items_shown > 0)
                                        remaining -= sep_len;

                                if (remaining < 4)
                                        break;

                                size_t display_chars = remaining - 3;

                                if (display_chars < 1)
                                        display_chars = 1;

                                if (items_shown > 0)
                                        printf(" | ");

                                if (i == st->selected_idx)
                                        printf(YELLOW BOLD INVERT);

                                printf("%.*s...",
                                       (int)display_chars,
                                       name);

                                if (i == st->selected_idx)
                                        printf(RESET);

                                break;
                        }

                        if (items_shown > 0)
                                printf(" | ");

                        if (i == st->selected_idx)
                                printf(YELLOW BOLD INVERT);

                        printf("%s", name);

                        if (i == st->selected_idx)
                                printf(RESET);

                        cursor_x = would_be;
                        items_shown++;
                }
        }

        /* Cursor position */
        gotoxy(strlen(label)
               + strlen(" [ ")
               + str_len(&st->input),
               win->h);

        fflush(stdout);
}

static char *
completion_run(window     *win,
               const char *label,
               cstr_array  items)
{
        completion_state st;
        st.input = str_create();
        st.selected_idx = 0;
        st.offset = 0;

        while (1) {
                char ch;
                input_type ty;

                cstr_array matches =
                        fuzzy_find(items, str_cstr(&st.input));

                size_t total_matches = matches.len;

                if (total_matches > MAX_COMPLETIONS_REQUEST)
                        total_matches = MAX_COMPLETIONS_REQUEST;

                completion_draw(win,
                                label,
                                &st,
                                matches.data,
                                total_matches);

                ty = get_input(&ch);

                switch (ty) {
                case INPUT_TYPE_NORMAL:
                        if (ENTER(ch)) {
                                if (total_matches > 0 &&
                                    st.selected_idx < total_matches) {
                                        char *res =
                                                strdup(matches.data[st.selected_idx]);
                                        dyn_array_free(matches);
                                        str_destroy(&st.input);
                                        return res;
                                }
                        } else if (BACKSPACE(ch)) {
                                if (str_len(&st.input) > 0) {
                                        str_pop(&st.input);
                                        st.selected_idx = 0;
                                        st.offset = 0;
                                }
                        } else if (isprint(ch) || ch == ' ') {
                                str_append(&st.input, ch);
                                st.selected_idx = 0;
                                st.offset = 0;
                        }
                        break;

                case INPUT_TYPE_CTRL:
                        if (ch == CTRL_N) {
                                if (total_matches > 0 &&
                                    st.selected_idx < total_matches - 1)
                                        st.selected_idx++;
                        } else if (ch == CTRL_P) {
                                if (total_matches > 0 &&
                                    st.selected_idx > 0)
                                        st.selected_idx--;
                        } else if (ch == CTRL_G) {
                                dyn_array_free(matches);
                                str_destroy(&st.input);
                                return NULL;
                        }
                        break;

                default: break;
                }

                dyn_array_free(matches);
        }
}

window
window_create(size_t w, size_t h)
{
        return (window) {
                .ab   = NULL,
                .abi  = 0,
                .bfrs = dyn_array_empty(bufferp_array),
                .w    = w,
                .h    = h,
                .compile = NULL,
                .pb      = NULL,
                .pbi     = 0,
        };
}

static buffer *
buffer_exists_by_name(window     *win,
                      const char *name)
{
        for (size_t i = 0; i < win->bfrs.len; ++i) {
                if (!strcmp(name, str_cstr(&win->bfrs.data[i]->name)))
                        return win->bfrs.data[i];
        }
        return NULL;
}

void
window_add_buffer(window *win,
                  buffer *b,
                  int     make_curr)
{
        if (!buffer_exists_by_name(win, str_cstr(&b->name)))
                dyn_array_append(win->bfrs, b);
        if (make_curr) {
                win->pb  = win->ab;
                win->pbi = win->abi;
                win->ab  = b;
                win->abi = win->bfrs.len-1;
        }
}

static void
change_buffer_by_name(window     *win,
                      const char *name)
{
        for (size_t i = 0; i < win->bfrs.len; ++i) {
                if (!strcmp(str_cstr(&win->bfrs.data[i]->name), name)) {
                        win->pb = win->ab;
                        win->pbi = win->abi;
                        win->abi = i;
                        win->ab = win->bfrs.data[i];
                        break;
                }
        }
}

static void
close_buffer(window *win)
{
        dyn_array_rm_at(win->bfrs, win->abi);

        if (win->bfrs.len == 0) {
                win->ab = NULL;
                win->abi = 0;
                return;
        }
        else if (win->abi > 0)
                --win->abi;

        win->ab = win->bfrs.data[win->abi];

        if (win->ab)
                buffer_dump(win->ab);

        win->pb = win->bfrs.data[0];
        win->pbi = 0;
}

static void
quit(window *win)
{
        while (win->ab)
                close_buffer(win);
}

static int
save_buffer(window *win)
{
        if (!buffer_save(win->ab))
                return 0;
}

void
find_file(window *win)
{
        cstr_array files;
        char *chosen_file = NULL;
        str cwd = str_from(".");

 reload_dir:

        files = lsdir(str_cstr(&cwd));
        char *selected = completion_run(win, "Find File", files);

        if (!selected)
                goto done;

        if (strcmp(selected, "..") == 0) {
                //char *slash = strrchr(str_cstr(&cwd), '/');

                str_concat(&cwd, "/..");

                /*if (slash && slash != str_cstr(&cwd))
                        *slash = '\0';
                else {
                        str_destroy(&cwd);
                        cwd = str_from(".");
                }*/

                free(selected);
                dyn_array_free(files);
                goto reload_dir;
        }

        str fullpath = str_from_fmt("%s/%s",
                                    str_cstr(&cwd),
                                    selected);

        if (is_dir(str_cstr(&fullpath))) {
                str_destroy(&cwd);
                cwd = fullpath;

                free(selected);
                dyn_array_free(files);
                goto reload_dir;
        }

        chosen_file = strdup(str_cstr(&fullpath));

        str_destroy(&fullpath);
        free(selected);

 done:
        dyn_array_free(files);
        str_destroy(&cwd);

        if (!chosen_file) {
                buffer_dump(win->ab);
                return;
        }

        str     fp = str_from(chosen_file);
        buffer *b  = NULL;

        if (!(b = buffer_exists_by_name(win, chosen_file))) {
                char *real = get_realpath(str_cstr(&fp));
                if (real)
                        b = buffer_from_file(str_from(real), win);
                else
                        b = buffer_from_file(fp, win);
        }

        free(chosen_file);

        if (!b)
                return;

        window_add_buffer(win, b, 1);
        buffer_dump(win->ab);
}

static void
choose_buffer(window *win)
{
        cstr_array names = dyn_array_empty(cstr_array);

        for (size_t i = 0; i < win->bfrs.len; ++i)
                dyn_array_append(names, strdup(str_cstr(&win->bfrs.data[i]->name)));

        char *selected = completion_run(win, "Switch Buffer", names);

        if (!selected)
                goto done;

        change_buffer_by_name(win, selected);
        free(selected);

 done:
        for (size_t i = 0; i < names.len; ++i)
                free(names.data[i]);
        dyn_array_free(names);

        buffer_dump(win->ab);
}

static char **
make_command(str *s)
{
        cstr_array ar = dyn_array_empty(cstr_array);
        char_array buf = dyn_array_empty(char_array);

        for (size_t i = 0; i < s->len; ++i) {
                char ch = str_at(s, i);
                if (ch == ' ') {
                        if (buf.len > 0) {
                                dyn_array_append(buf, 0);
                                dyn_array_append(ar, strdup(buf.data));
                                dyn_array_clear(buf);
                        }
                } else {
                        dyn_array_append(buf, ch);
                }
        }

        if (buf.len > 0) {
                dyn_array_append(buf, 0);
                dyn_array_append(ar, strdup(buf.data));
        }

        dyn_array_append(ar, NULL);
        dyn_array_free(buf);

        return ar.data;
}

static char *
capture_command_output(str *input)
{
        int pipefd[2];
        pid_t pid;
        char *output = NULL;
        size_t output_size = 0;
        size_t output_cap = 0;
        char buffer[4096];

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
        while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[n] = '\0';

                size_t needed = output_size + n + 1;
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

                memcpy(output + output_size, buffer, n);
                output_size += n;
                output[output_size] = '\0';
        }

        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);

        return output;
}

#define COMPILATION_HEADER "*** " BOLD WHITE "Compilation" RESET " [ " BOLD YELLOW "%s" RESET " ] [ (q)uit, a(g)ain, M-<tab>:switch-here ] ***\n\n"
static void
do_compilation(window *win)
{
        if (!win->compile)
                return;

        str input = str_from(win->compile);
        buffer *b = NULL;
        int exists = 0;

        for (size_t i = 0; i < win->bfrs.len; ++i) {
                if (!strcmp(str_cstr(&win->bfrs.data[i]->name), "ww-compilation")) {
                        b = win->bfrs.data[i];
                        win->ab = b;
                        win->abi = i;
                        break;
                }
        }

        if (!b) {
                b = buffer_alloc(win);
                str_destroy(&b->name);
                b->name = str_from("ww-compilation");
                b->writable = 0;
        } else {
                exists = 1;
                for (size_t i = 0; i < b->lns.len; ++i)
                        line_free(b->lns.data[i]);
                dyn_array_free(b->lns);
        }

        if (!exists)
                window_add_buffer(win, b, 1);
        buffer_dump(win->ab);

        char *output = capture_command_output(&input);
        win->ab->lns = lines_of_cstr(output);

        char buf[1024] = {0};
        sprintf(buf, COMPILATION_HEADER, str_cstr(&input));
        line_array header = lines_of_cstr(buf);
        for (int i = header.len-1; i >= 0; --i)
                dyn_array_insert_at(win->ab->lns, 0, header.data[i]);

        dyn_array_append(win->ab->lns, line_from(str_from("\n")));
        dyn_array_append(win->ab->lns, line_from(str_from("[ Done ] ")));
        win->ab->cx = 0;
        win->ab->al = 0;
        win->ab->cy = 0;
        adjust_scroll(win->ab);

        str_destroy(&input);
        buffer_dump(win->ab);
}

static str
get_generic_input(window     *win,
                  const char *input_start,
                  const char *prompt)
{
        str input;

        if (input_start)
                input = str_from(input_start);
        else
                input = str_create();

        while (1) {
                gotoxy(0, win->h);
                clear_line(0, win->h);
                printf("%s [ %s", prompt ? prompt : "", str_cstr(&input));
                fflush(stdout);

                char       ch;
                input_type ty;

                ty = get_input(&ch);
                switch (ty) {
                case INPUT_TYPE_NORMAL:
                        if (ch == '\n')
                                goto done;
                        else if (BACKSPACE(ch))
                                str_pop(&input);
                        else
                                str_append(&input, ch);
                        break;
                case INPUT_TYPE_CTRL:
                        if (ch == CTRL_G) {
                                str_clear(&input);
                                goto done;
                        }
                        break;
                default: break;
                }
        }

done:
        return input;
}

static void
compilation_buffer(window *win)
{
        const char *input_start;

        if (win->compile)
                input_start = win->compile;
        else if (glconf.defaults.compile_cmd)
                input_start = glconf.defaults.compile_cmd;
        else
                input_start = NULL;

        str input = get_generic_input(win, input_start, "Compile");

        if (input.len <= 0) {
                str_destroy(&input);
                buffer_dump(win->ab);
                return;
        }

        if (win->compile)
                free(win->compile);

        win->compile = strdup(str_cstr(&input));
        do_compilation(win);
}

static void
find_replace(window *win)
{
        str from = get_generic_input(win, NULL, "Replace From");

        if (from.len == 0)
                return;

        win->ab->state = BS_SEARCH;

        str last_search = win->ab->last_search;
        win->ab->last_search = from;

        buffer_dump(win->ab);
        str to = get_generic_input(win, NULL, "Replace To");

        win->ab->state = BS_SELECTION;

        win->ab->last_search = last_search;

        if (to.len == 0) {
                str_destroy(&from);
                return;
        }

        buffer_find_and_replace_in_selection(win->ab, str_cstr(&from), str_cstr(&to));
}

static void
set_space_amt(window *win)
{
        str input;

        input = get_generic_input(win, NULL, "Space#");

        if (!input.len)
                return;
        if (!cstr_isdigit(str_cstr(&input)))
                goto cleanup;

        glconf.defaults.space_amt = atoi(str_cstr(&input));

cleanup:
        str_destroy(&input);
}

void
window_open_help_buffer(window *win)
{
        line_array help;
        line_array controls;

        help = lines_of_cstr(HELP_DEF);
        controls = lines_of_cstr(CONTROLS_DEF);

        buffer *b = NULL;

        for (size_t i = 0; i < win->bfrs.len; ++i) {
                if (!strcmp(str_cstr(&win->bfrs.data[i]->name), "ww-compilation")) {
                        b = win->bfrs.data[i];
                        win->ab = b;
                        win->abi = i;
                        break;
                }
        }

        if (!b) {
                b = buffer_alloc(win);
                str_destroy(&b->name);
                b->name = str_from("ww-help");
                b->writable = 0;
                window_add_buffer(win, b, 1);
        }

        win->ab->lns = help;

        for (size_t i = 0; i < controls.len; ++i)
                dyn_array_append(win->ab->lns, controls.data[i]);

        win->ab->cx  = 0;
        win->ab->al  = 0;
        win->ab->cy  = 0;

        adjust_scroll(win->ab);
}

static void
metax(window *win)
{
        // TODO: make `names' live in static memory
        //       since we don't need to always refill it.

        cstr_array  names;
        char       *cmds[] = WINCMDS;
        char       *selected;

        names = dyn_array_empty(cstr_array);

        for (size_t i = 0; i < sizeof(cmds)/sizeof(*cmds); ++i)
                dyn_array_append(names, cmds[i]);

        if (!(selected = completion_run(win, "M-x", names))) {
                buffer_dump(win->ab);
                goto done;
        }

        if (!strcmp(selected, WINCMD_SPCAMT)) {
                set_space_amt(win);
                buffer_dump(win->ab);
        } else if (!strcmp(selected, WINCMD_KILLBUF)) {
                close_buffer(win);
        } else if (!strcmp(selected, WINCMD_SWTCHBUF)) {
                choose_buffer(win);
        } else if (!strcmp(selected, WINCMD_COMP)) {
                compilation_buffer(win);
        } else if (!strcmp(selected, WINCMD_SAVEBUF)) {
                buffer_dump(win->ab);
                buffer_save(win->ab);
        } else if (!strcmp(selected, WINCMD_EXIT)) {
                quit(win);
        } else if (!strcmp(selected, WINCMD_COPYBUFTOCLIP)) {
                buffer_copybuf_to_clipboard(win->ab);
        } else if (!strcmp(selected, WINCMD_TABMODE)) {
                glconf.flags &= ~FT_SPACESARETABS;
                buffer_dump(win->ab);
        } else if (!strcmp(selected, WINCMD_SPACEMODE)) {
                glconf.flags |= FT_SPACESARETABS;
                buffer_dump(win->ab);
        } else if (!strcmp(selected, WINCMD_REPLACE)) {
                find_replace(win);
                buffer_dump(win->ab);
        } else if (!strcmp(selected, WINCMD_HELP)) {
                window_open_help_buffer(win);
                buffer_dump(win->ab);
        } else if (!strcmp(selected, WINCMD_FINDFILE)) {
                find_file(win);
        } else if (!strcmp(selected, WINDCMD_TRAILMODE)) {
                glconf.flags ^= FT_SHOWTRAILS;
                buffer_dump(win->ab);
        } else {
                assert(0 && "unknown M-x command");
        }

done:
        dyn_array_free(names);
}

static int
ctrlx(window *win)
{
        char        ch;
        input_type  ty;

        switch (ty = get_input(&ch)) {
        case INPUT_TYPE_NORMAL: {
                if (ch == 'k')
                        close_buffer(win);
                if (ch == 'b')
                        choose_buffer(win);
                if (ch == 'x')
                        compilation_buffer(win);
        } break;
        case INPUT_TYPE_CTRL:
                if (ch == CTRL_S)
                        save_buffer(win);
                if (ch == CTRL_Q)
                        quit(win);
                if (ch == CTRL_F)
                        find_file(win);
        default: break;
        }

        return 1;
}

static void
try_jump_to_error(window *win)
{
        const line *ln       = NULL;
        char       *filename = NULL;
        int         row      = -1;
        int         col      = -1;

        ln = win->ab->lns.data[win->ab->al];

        regex_t regex;
        regmatch_t matches[4]; // full match + 3 capture groups

        const char *pattern = "([^[:space:]:]+):([0-9]+):([0-9]+):";

        if (regcomp(&regex, pattern, REG_EXTENDED)) {
                fprintf(stderr, "Could not compile regex\n");
                return;
        }

        if (regexec(&regex, ln->s.chars, 4, matches, 0) == 0) {
                int fname_len = matches[1].rm_eo - matches[1].rm_so;

                if (!(filename = malloc(fname_len + 1))) {
                        regfree(&regex);
                        return;
                }

                memcpy(filename,
                       ln->s.chars + matches[1].rm_so,
                       fname_len);
                filename[fname_len] = '\0';

                row = atoi(ln->s.chars + matches[2].rm_so);
                col = atoi(ln->s.chars + matches[3].rm_so);
        }

        regfree(&regex);

        if (!filename) {
                buffer_dump(win->ab);
                return;
        }

        if (row == -1 || col == -1) {
                free(filename);
                buffer_dump(win->ab);
                return;
        }

        buffer *b = NULL;

        if (!(b = buffer_exists_by_name(win, filename))) {
                char *real = get_realpath(filename);
                if (real)
                        b = buffer_from_file(str_from(real), win);
                else
                        b = buffer_from_file(str_from(filename), win);
        }

        window_add_buffer(win, b, 1);
        buffer_jump_to_verts(win->ab, col-1, row-1);

        free(filename);
        buffer_dump(win->ab);
}

void
window_handle(window *win)
{
        assert(win->ab);

        buffer_dump(win->ab);
        gotoxy(win->ab->cx, win->ab->cy);
        fflush(stdout);

        while (1) {
                if (!win->ab)
                        break;

                if (!win->pb) {
                        win->pb = win->ab;
                        win->pbi = win->abi;
                }

                char        ch;
                input_type  ty;
                buffer_proc bproc;

                ty = get_input(&ch);
                int is_compilation = !strcmp(str_cstr(&win->ab->name), "ww-compilation");

                if (ty == INPUT_TYPE_NORMAL && ch == '\n' && is_compilation) {
                        try_jump_to_error(win);
                } else if (ty == INPUT_TYPE_NORMAL && ch == 'q' && is_compilation) {
                        win->ab = win->pb;
                        win->abi = win->pbi;
                        buffer_dump(win->ab);
                } else if (ty == INPUT_TYPE_ALT && ch == '\t' && !is_compilation) {
                        change_buffer_by_name(win, "ww-compilation");
                        buffer_dump(win->ab);
                } else if (ty == INPUT_TYPE_NORMAL && ch == 'g' && is_compilation)
                        do_compilation(win);
                else if (ty == INPUT_TYPE_ALT && ch == 'x')
                        metax(win);
                else if (ty == INPUT_TYPE_CTRL && ch == CTRL_X)
                        ctrlx(win);
                else {
                        bproc = buffer_process(win->ab, ty, ch);

                        if (bproc == BP_INSERT)
                                buffer_dump_xy(win->ab);
                        else if (bproc == BP_INSERTNL)
                                buffer_dump(win->ab);
                        else if (bproc == BP_MOV)
                                buffer_dump_xy(win->ab);
                                //buffer_dump(win->ab);
                }
        }
}
