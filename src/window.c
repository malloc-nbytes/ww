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

static int
save_buffer(window *win)
{
        if (!buffer_save(win->ab))
                return 0;
}

void
find_file(window *win)
{
        cstr_array   files;
        void        *trie;
        size_t       out_count;
        char        *chosen_file;
        str          inbuf;

        files       = lsdir(".");
        trie        = trie_alloc();
        out_count   = 0;
        chosen_file = NULL;
        inbuf       = str_create();

        for (size_t i = 0; i < files.len; ++i)
                (void)trie_insert(trie, files.data[i]);

        while (1) {
                char         ch;
                input_type   ty;
                char       **completions = NULL;
                size_t       out_count = 0;
                static size_t selected_idx = 0;

                completions = trie_get_completions(trie, str_cstr(&inbuf), 5, &out_count);

                if (out_count > 0 && selected_idx >= out_count)
                        selected_idx = 0;

                gotoxy(0, win->h);
                clear_line(0, win->h);
                printf("Find File [ %s", str_cstr(&inbuf));

                gotoxy(0, win->h-1);
                clear_line(0, win->h-1);

                for (size_t i = 0; i < out_count; ++i) {
                        if (i > 0) putchar(' ');

                        if (i == selected_idx) {
                                printf(YELLOW BOLD);
                        }

                        printf("%s", completions[i] ? completions[i] : "?");

                        if (i == selected_idx)
                                printf(RESET);
                }

                gotoxy(strlen("Find File [ ") + str_len(&inbuf), win->h);
                fflush(stdout);

                ty = get_input(&ch);

                switch (ty) {
                case INPUT_TYPE_NORMAL:
                        if (ENTER(ch)) {
                                if (out_count > 0 && selected_idx < out_count)
                                        chosen_file = strdup(completions[selected_idx]);
                                free(completions);
                                goto done;
                        }
                        else if (BACKSPACE(ch)) {
                                if (str_len(&inbuf) > 0)
                                        str_pop(&inbuf);
                                selected_idx = 0;
                        }
                        else if (isprint(ch) || ch == ' ') {
                                str_append(&inbuf, ch);
                                selected_idx = 0;
                        }
                        break;

                case INPUT_TYPE_CTRL:
                        if (ch == CTRL_N) {
                                if (out_count > 0)
                                        selected_idx = (selected_idx + 1) % out_count;
                        } else if (ch == CTRL_P) {
                                if (out_count > 0)
                                        selected_idx = (selected_idx == 0) ? out_count-1 : selected_idx-1;
                        }
                        break;

                default:
                        break;
                }

                free(completions);
        }

done:
        if (!chosen_file)
                return;

        assert(!is_dir(chosen_file));

        str     fp;
        buffer *b;

        fp = str_from(chosen_file);
        if (!(b = buffer_from_file(fp, win)))
                return;

        window_add_buffer(win, b, 1);
        buffer_dump(win->ab);
}

static int
ctrlx(window *win)
{
        char        ch;
        input_type  ty;

        switch (ty = get_input(&ch)) {
        case INPUT_TYPE_CTRL:
                if (ch == CTRL_S)
                        return save_buffer(win);
                if (ch == CTRL_Q)
                        close_buffer(win);
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
