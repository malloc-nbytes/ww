#include "minibuffer.h"
#include "term.h"
#include "buffer.h"
#include "glconf.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

char *
minibuffer_input(const char *prompt,
                 const str  *autofill,
                 str_ar     *ac_items)
{
        gotoxy(0, (unsigned)glconf.term.h);

        int    first;
        str    buf;
        size_t prompt_n;
        size_t cx;

        first    = 1;
        buf      = str_create();
        prompt_n = prompt ? strlen(prompt) : 0;
        cx       = 0;

        while (1) {
                clear_line(0, glconf.term.h);
                printf("%s [ %s", prompt ? prompt : "", buf.chars);
                gotoxy(cx + prompt_n + strlen(" [ "), (unsigned)glconf.term.h);
                fflush(stdout);

                if (first) {
                        if (autofill)
                                str_overwrite(&buf, autofill->chars);
                        else
                                buf = str_create();
                        first = 0;
                        cx    = buf.len > 0 ? buf.len : 0;
                }

                char       ch;
                input_type ty;

                switch (ty = get_input(&ch)) {
                case INPUT_TYPE_NORMAL: {
                        if (BACKSPACE(ch)) {
                                if (buf.len > 0) {
                                        str_rm(&buf, cx-1);
                                        --cx;
                                }
                        } else if (ch == '\n')
                                goto done;
                        else {
                                str_insert(&buf, cx, ch);
                                ++cx;
                        }
                } break;
                case INPUT_TYPE_CTRL: {
                        if (ch == CTRL_Y) {
                                if (g_cpy_buf.len > 0) {
                                        char cpy[1024] = {0};
                                        memcpy(cpy, g_cpy_buf.data, g_cpy_buf.len);
                                        str_concat(&buf, g_cpy_buf.data);
                                        cx = buf.len;
                                }
                        } else if (ch == CTRL_B && cx > 0)
                                --cx;
                        else if (ch == CTRL_F && cx < buf.len)
                                ++cx;
                        else if (ch == CTRL_A)
                                cx = 0;
                        else if (ch == CTRL_E)
                                cx = buf.len;
                } break;
                case INPUT_TYPE_ALT: {
                } break;
                case INPUT_TYPE_ARROW: {
                } break;
                default: break;
                }
        }

 done:
        return buf.len > 0 ? buf.chars : NULL;
}
