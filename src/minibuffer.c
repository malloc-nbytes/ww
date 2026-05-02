#include "ww.h"
#include "minibuffer.h"
#include "term.h"
#include "buffer.h"
#include "colors.h"
#include "fuzzy.h"
#include "glconf.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_COMPLETIONS           200
#define DISPLAY_COUNT             6
#define MAX_COMPLETIONS_REQUEST   200
#define MAX_VERTICAL_LINES        8
#define FILE_SELECTION_PADDING    10

typedef struct {
        str    input;
        size_t selected_idx;
        size_t offset;
} completion_state;

static void
completion_draw(ww                *ed,
                const char        *label,
                completion_state  *st,
                char             **completions,
                size_t             total_matches)
{
        (void)ed;

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
        gotoxy(0, (unsigned)glconf.term.h);
        clear_line(0, glconf.term.h);

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
        gotoxy(0, (unsigned)glconf.term.h - 1);
        clear_line(0, glconf.term.h - 1);

        if (total_matches > 0) {
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

                        if (would_be > glconf.term.w - FILE_SELECTION_PADDING) {
                                size_t remaining = glconf.term.w - cursor_x;

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
                                printf("%s", YELLOW BOLD INVERT);

                        printf("%s", name);

                        if (i == st->selected_idx)
                                printf(RESET);

                        cursor_x = would_be;
                        items_shown++;
                }
        }

        /* Cursor position */
        gotoxy((unsigned)strlen(label)
               + (unsigned)strlen(" [ ")
               + (unsigned)str_len(&st->input),
               (unsigned)glconf.term.h);

        fflush(stdout);
}

char *
minibuffer_input(ww         *ed,
                 const char *label,
                 const char *autofill,
                 cstr_ar     items)
{
        completion_state st;
        size_t           cx;

        st.selected_idx = 0;
        st.offset       = 0;

        if (autofill) {
                st.input = str_from(autofill);
                cx       = st.input.len;
        } else {
                st.input = str_create();
                cx       = 0;
        }


        while (1) {
                char ch;
                input_type ty;

                cstr_ar matches =
                        fuzzy_find(items, str_cstr(&st.input));

                size_t total_matches = matches.len;
                if (total_matches > MAX_COMPLETIONS_REQUEST)
                        total_matches = MAX_COMPLETIONS_REQUEST;

                completion_draw(ed,
                                label,
                                &st,
                                matches.data,
                                total_matches);

                gotoxy((unsigned)(cx + (label?strlen(label):0) + strlen(" [ ")),
                       (unsigned)glconf.term.h);
                fflush(stdout);

                ty = get_input(&ch);

                switch (ty) {
                case INPUT_TYPE_NORMAL: {
                        if (ENTER(ch)) {
                                char *res;
                                if (total_matches > 0 &&
                                    st.selected_idx < total_matches) {
                                        res = strdup(matches.data[st.selected_idx]);
                                } else {
                                        res = strdup(str_cstr(&st.input));
                                }

                                array_free(matches);
                                str_destroy(&st.input);
                                return res;
                        }
                        else if (BACKSPACE(ch)) {
                                if (cx > 0 && st.input.len > 0) {
                                        str_rm(&st.input, cx - 1);
                                        cx--;
                                        st.selected_idx = 0;
                                        st.offset       = 0;
                                }
                        }
                        else if (isprint(ch) || ch == ' ') {
                                str_insert(&st.input, cx, ch);
                                cx++;
                                st.selected_idx = 0;
                                st.offset       = 0;
                        }
                } break;

                case INPUT_TYPE_CTRL: {
                        if (ch == CTRL_N) {
                                if (total_matches > 0 &&
                                    st.selected_idx < total_matches - 1)
                                        st.selected_idx++;
                        } else if (ch == CTRL_P) {
                                if (total_matches > 0 &&
                                    st.selected_idx > 0)
                                        st.selected_idx--;
                        } else if (ch == CTRL_G) {
                                array_free(matches);
                                str_destroy(&st.input);
                                return NULL;
                        } else if (ch == CTRL_B && cx > 0) {
                                --cx;
                        } else if (ch == CTRL_F && cx < st.input.len) {
                                ++cx;
                        } else if (ch == CTRL_A) {
                                cx = 0;
                        } else if (ch == CTRL_E) {
                                cx = st.input.len;
                        } else if (ch == CTRL_D) {
                                str_rm(&st.input, cx);
                        } else if (ch == CTRL_K) {
                                str_cut(&st.input, cx);
                        } else if (ch == CTRL_Y) {
                                if (g_cpy_buf.len > 0) {
                                        char cpy[1024] = {0};
                                        memcpy(cpy, g_cpy_buf.data, g_cpy_buf.len);
                                        str_concat(&st.input, g_cpy_buf.data);

                                        cx += g_cpy_buf.len;

                                        st.selected_idx = 0;
                                        st.offset       = 0;
                                }
                        }
                } break;
                default:
                        break;
                }

                array_free(matches);
        }
}
