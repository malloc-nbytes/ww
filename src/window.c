#include "window.h"
#include "term.h"
#include "io.h"
#include "trie.h"
#include "str.h"
#include "colors.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

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
completion_run(window *win,
               const char *label,
               void *trie)
{
        /* Runs completion UI. Returns selected string or NULL. */

        completion_state st;
        st.input = str_create();
        st.selected_idx = 0;
        st.offset = 0;

        while (1) {
                char ch;
                input_type ty;

                size_t total_matches = 0;

                char **completions = trie_get_completions(trie,
                                                          str_cstr(&st.input),
                                                          MAX_COMPLETIONS_REQUEST,
                                                          &total_matches);

                completion_draw(win,
                                label,
                                &st,
                                completions,
                                total_matches);

                ty = get_input(&ch);

                switch (ty) {
                case INPUT_TYPE_NORMAL:
                        if (ENTER(ch)) {
                                if (total_matches > 0 &&
                                    st.selected_idx < total_matches) {
                                        char *res = strdup(completions[st.selected_idx]);
                                        free(completions);
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
                                if (total_matches > 0
                                    && st.selected_idx < total_matches - 1)
                                        st.selected_idx++;
                        } else if (ch == CTRL_P) {
                                if (total_matches > 0
                                    && st.selected_idx > 0)
                                        st.selected_idx--;
                        } else if (ch == CTRL_G) {
                                free(completions);
                                str_destroy(&st.input);
                                return NULL;
                        }
                        break;

                default: break;
                }

                free(completions);
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
        };
}

void
window_add_buffer(window *win,
                  buffer *b,
                  int     make_curr)
{
        dyn_array_append(win->bfrs, b);
        if (make_curr) {
                win->ab  = b;
                win->abi = win->bfrs.len-1;
        }
}

static void
change_buffer_by_name(window     *win,
                      const char *name)
{
        for (size_t i = 0; i < win->bfrs.len; ++i) {
                if (!strcmp(str_cstr(&win->bfrs.data[i]->filename), name)) {
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
        void *trie;
        char *chosen_file = NULL;
        str cwd = str_from(".");

 reload_dir:

        files = lsdir(str_cstr(&cwd));
        trie = trie_alloc();

        for (size_t i = 0; i < files.len; ++i)
                trie_insert(trie, files.data[i]);

        char *selected =
                completion_run(win,
                               "Find File",
                               trie);

        if (!selected)
                goto done;

        if (strcmp(selected, "..") == 0) {
                char *slash = strrchr(str_cstr(&cwd), '/');

                if (slash && slash != str_cstr(&cwd))
                        *slash = '\0';
                else {
                        str_destroy(&cwd);
                        cwd = str_from(".");
                }

                free(selected);
                trie_destroy(trie);
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
                trie_destroy(trie);
                dyn_array_free(files);
                goto reload_dir;
        }

        chosen_file = strdup(str_cstr(&fullpath));

        str_destroy(&fullpath);
        free(selected);

 done:
        trie_destroy(trie);
        dyn_array_free(files);
        str_destroy(&cwd);

        if (!chosen_file) {
                buffer_dump(win->ab);
                return;
        }

        str fp = str_from(chosen_file);
        buffer *b = buffer_from_file(fp, win);
        free(chosen_file);

        if (!b)
                return;

        window_add_buffer(win, b, 1);
        buffer_dump(win->ab);
}

static void
choose_buffer(window *win)
{
        void *trie = trie_alloc();

        for (size_t i = 0; i < win->bfrs.len; ++i)
                trie_insert(trie, str_cstr(&win->bfrs.data[i]->filename));

        char *selected = completion_run(win, "Switch Buffer", trie);

        if (!selected)
                goto done;

        change_buffer_by_name(win, selected);
        free(selected);

 done:
        trie_destroy(trie);
        buffer_dump(win->ab);
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
        } break;
        case INPUT_TYPE_CTRL:
                if (ch == CTRL_S)
                        return save_buffer(win);
                if (ch == CTRL_Q)
                        quit(win);
                if (ch == CTRL_F)
                        find_file(win);
        default: break;
        }

        return 1;
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

                char        ch;
                input_type  ty;
                buffer_proc bproc;

                ty = get_input(&ch);

                if (ty == INPUT_TYPE_ALT && ch == 'x')
                        assert(0);
                else if (ty == INPUT_TYPE_CTRL && ch == CTRL_X)
                        ctrlx(win);
                else {
                        bproc = buffer_process(win->ab, ty, ch);

                        if (bproc == BP_INSERT)
                                buffer_dump_xy(win->ab);
                        else if (bproc == BP_INSERTNL)
                                buffer_dump(win->ab);
                        else if (bproc == BP_MOV)
                                buffer_dump(win->ab);
                }
        }
}
