#include "buffer.h"
#include "mem.h"
#include "term.h"

#include <assert.h>
#include <stdio.h>

buffer *
buffer_from(str      name,
            str      path,
            unsigned w,
            unsigned h,
            unsigned ws,
            unsigned hs,
            line_ar  lns)
{
        buffer *b;

        b          = (buffer *)alloc(sizeof(buffer));
        b->name    = name;
        b->path    = path;
        b->size.w  = w;
        b->size.h  = h;
        b->size.ws = ws;
        b->size.hs = hs;
        b->lines   = lns;

        return b;
}

static unsigned
get_win_hight(const buffer *b)
{
        return b->size.h-1;
}

static int
adjust_vscroll(buffer *b)
{
        size_t win_h = get_win_hight(b);

        if (b->cy < b->voff) {
                b->voff = b->cy;
                return 1;
        } else if (b->cy >= b->voff + win_h) {
                b->voff = b->cy - win_h + 1;
                return 1;
        }
        return 0;
}

static int
adjust_hscroll(buffer *b)
{
        size_t win_w = get_win_hight(b);

        if (b->cx < b->hoff) {
                b->hoff = b->cx;
                return 1;
        } else if (b->cx >= b->hoff + win_w) {
                b->hoff = b->cx - win_w + 1;
                return 1;
        }
        return 0;
}

static buffer_action
adjust_scroll(buffer *b)
{
        int res;

        res = adjust_vscroll(b);
        res = adjust_hscroll(b) || res;

        return res ? BA_REDRAW : BA_NOP;
}

static buffer_action
up(buffer *b)
{
        if (b->cy > 0) {
                --b->cy;
                --b->al;
        }

        gotoxy((unsigned)(b->cx - b->hoff), (unsigned)(b->cy - b->voff));
        return adjust_scroll(b);
}

static buffer_action
down(buffer *b)
{
        if (b->cy < b->lines.len-1) {
                ++b->cy;
                ++b->al;
        }

        gotoxy((unsigned)(b->cx - b->hoff), (unsigned)(b->cy - b->voff));
        return adjust_scroll(b);
}

buffer_action
buffer_process(buffer *b)
{
        input_type ty;
        char ch;

        switch (ty = get_input(&ch)) {
        case INPUT_TYPE_NORMAL: assert(0);
        case INPUT_TYPE_CTRL: {
                if (ch == CTRL_N) {
                        return down(b);
                } else if (ch == CTRL_P) {
                        return up(b);
                }
        } break;
        default: break;
        }

        return BA_NOP;
}

static void
drawln(const buffer *b, size_t idx)
{
        if (idx >= b->lines.len) {
                // Empty line
                clear_line_imm();
                return;
        }

        const line *ln = &b->lines.data[idx];
        const str *s = &ln->txt;

        // determine visible portion after horizontal offset
        size_t start = b->hoff;
        if (start >= s->len) {
                clear_line_imm();
                return;
        }

        size_t visible_len = s->len - start;

        // print the visible slice
        fwrite(s->chars + start, 1, visible_len, stdout);
        // printf("%.*s", (int)visible_len, s->chars + start);

        clear_line_imm();
}

void
buffer_drawxy(const buffer *b)
{
        // compute screen position of the cursor, accounting for scroll offsets and window start
        unsigned screen_x = b->size.ws + (unsigned)(b->cx);
        unsigned screen_y = b->size.hs + (unsigned)(b->cy);

        // Move cursor to the start of the visible cursor line on screen
        gotoxy(screen_x, screen_y);

        // Now draw the remainder of the logical line starting at the cursor's x
        if (b->cy >= b->lines.len) {
                clear_line_imm();
                return;
        }

        const line *ln = &b->lines.data[b->cy];
        const str *s = &ln->txt;

        if (b->cx >= s->len) {
                clear_line_imm();
                return;
        }

        size_t remaining = s->len - b->cx;
        fwrite(s->chars + b->cx, 1, remaining, stdout);
        clear_line_imm();

        // Optionally move cursor back to exact position if your draw moved it
        gotoxy(screen_x, screen_y);
}

void
buffer_draw(const buffer *b)
{
        // Move to top-left of this buffer's window
        gotoxy(b->size.ws, b->size.hs);

        // Determine how many screen rows this window has
        // (You may want to add window height/width fields to buffer or pass them.
        // For now we assume the caller manages the region size and we draw until
        // we run out of lines or the window ends naturally.)
        // A simple way: draw up to a reasonable max, or add `unsigned wh, ww;` to buffer.

        size_t visible_rows = b->size.h;

        for (size_t i = 0; i < visible_rows; ++i) {
                size_t file_row = b->voff + i;

                // Position cursor at start of this screen row
                gotoxy(b->size.ws, b->size.hs + (unsigned)i);

                if (file_row < b->lines.len) {
                        drawln(b, file_row);
                } else {
                        // Beyond end of file → clear the line (often shown as ~ in editors)
                        clear_line_imm();
                        // Optional: printf("~"); or similar for "empty" marker
                }
        }

        // After drawing the text, place the cursor at its correct screen position
        unsigned cursor_screen_x = b->size.ws + (unsigned)(b->cx - b->hoff);
        unsigned cursor_screen_y = b->size.hs + (unsigned)(b->cy - b->voff);

        gotoxy(cursor_screen_x, cursor_screen_y);

        fflush(stdout);
}
