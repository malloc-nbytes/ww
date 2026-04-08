#include "buffer.h"
#include "mem.h"
#include "term.h"

#include <assert.h>
#include <stdio.h>
#include <ctype.h>

#define ADJUST_CURSOR gotoxy((unsigned)(b->cx - b->hoff), (unsigned)(b->cy - b->voff));

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
        b->cx      = 0;
        b->cy      = 0;
        b->al      = 0;
        b->hoff    = 0;
        b->voff    = 0;

        return b;
}

static unsigned
get_win_hight(const buffer *b)
{
        return b->size.h > 0 ? b->size.h - 1 : 0;
}

static unsigned
get_win_width(const buffer *b)
{
        return b->size.w > 0 ? b->size.w : 80;
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
        size_t win_w = get_win_width(b);

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

        if (b->cx >= b->lines.data[b->al].txt.len)
                b->cx = (unsigned)b->lines.data[b->al].txt.len-1;

        ADJUST_CURSOR;
        return adjust_scroll(b);
}

static buffer_action
down(buffer *b)
{
        if (b->cy < b->lines.len-1) {
                ++b->cy;
                ++b->al;
        }

        if (b->cx >= b->lines.data[b->al].txt.len)
                b->cx = (unsigned)b->lines.data[b->al].txt.len-1;

        ADJUST_CURSOR;
        return adjust_scroll(b);
}

static buffer_action
right(buffer *b)
{
        str *s = &b->lines.data[b->al].txt;

        if (b->cx == str_len(s)-1 && b->cy < b->lines.len-1) {
                b->cx = 0;
                ++b->cy;
                ++b->al;
        } else if (b->cx < str_len(s)-1) {
                ++b->cx;
        }

        ADJUST_CURSOR;
        return adjust_scroll(b);
}

static buffer_action
left(buffer *b)
{
        if (b->cx == 0 && b->cy > 0) {
                b->cx = (unsigned)str_len(&b->lines.data[b->al-1].txt)-1;
                --b->cy;
                --b->al;
        }
        else if (b->cx > 0)
                --b->cx;

        ADJUST_CURSOR;
        return adjust_scroll(b);
}

static buffer_action
bol(buffer *b)
{
        b->cx = 0;
        ADJUST_CURSOR;
        return adjust_scroll(b);
}

static buffer_action
eol(buffer *b)
{
        b->cx = (unsigned)b->lines.data[b->al].txt.len-1;
        ADJUST_CURSOR;
        return adjust_scroll(b);
}

static buffer_action
jump_next_word(buffer *b)
{
        const line *ln;
        const str  *s;
        const char *sraw;
        int         hitchars;
        size_t      i;

        ln       = &b->lines.data[b->al];
        s        = &ln->txt;
        sraw     = str_cstr(s);
        hitchars = 0;
        i        = b->cx;

        if (str_len(s) <= 0)
                return BA_NOP;

        while (i < str_len(s)) {
                if (isalnum(sraw[i]))
                        hitchars = 1;
                else if (hitchars)
                        break;
                ++i;
        }

        if (i == str_len(s))
                b->cx = (unsigned)str_len(s)-1;
        else
                b->cx = (unsigned)i;

        ADJUST_CURSOR;
        return adjust_scroll(b);
}

static buffer_action
jump_prev_word(buffer *b)
{
        const line *ln;
        const str  *s;
        const char *sraw;
        int         hitchars;
        size_t      i;

        ln       = &b->lines.data[b->al];
        s        = &ln->txt;
        sraw     = str_cstr(s);
        hitchars = 0;
        i        = b->cx-1;

        if (str_len(s) == 0 || b->cx == 0)
                return BA_NOP;

        while (i > 0) {
                if (isalnum(sraw[i]))
                        hitchars = 1;
                else if (hitchars)
                        break;
                --i;
        }

        b->cx = (unsigned)i;

        if (!isalnum(sraw[b->cx]))
                ++b->cx;

        ADJUST_CURSOR;
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
                } else if (ch == CTRL_F) {
                        return right(b);
                } else if (ch == CTRL_B) {
                        return left(b);
                } else if (ch == CTRL_E) {
                        return eol(b);
                } else if (ch == CTRL_A) {
                        return bol(b);
                }
        } break;
        case INPUT_TYPE_ALT: {
                if (ch == 'f') {
                        return jump_next_word(b);
                } else if (ch == 'b') {
                        return jump_prev_word(b);
                }
        } break;
        default: break;
        }

        return BA_NOP;
}

static void
drawln(const buffer *b, size_t idx)
{
        if (idx < b->voff || idx >= b->voff + get_win_hight(b))
                return;

        const line *ln = &b->lines.data[idx];
        unsigned y = b->size.hs + (unsigned)(idx - b->voff);
        unsigned win_w = get_win_width(b);

        // Clear line
        gotoxy(b->size.ws, y);
        for (unsigned x = 0; x < win_w; ++x)
                putchar(' ');

        // Draw text with horizontal scroll
        if (b->hoff < ln->txt.len) {
                size_t start = b->hoff;
                size_t len = ln->txt.len - start;
                if (len > win_w)
                        len = win_w;

                gotoxy(b->size.ws, y);
                for (size_t i = 0; i < len; ++i)
                        putchar(ln->txt.chars[start + i]);
        }
}

void
buffer_drawxy(const buffer *b)
{
        drawln(b, b->cy);

        unsigned screen_x = b->size.ws + (unsigned)(b->cx > b->hoff ? b->cx - b->hoff : 0);
        unsigned screen_y = b->size.hs + (unsigned)(b->cy - b->voff);
        gotoxy(screen_x, screen_y);
}

void
buffer_draw(const buffer *b)
{
        unsigned win_w = get_win_width(b);
        unsigned win_h = get_win_hight(b);

        // Clear the entire buffer area
        for (unsigned y = 0; y < win_h; ++y) {
                gotoxy(b->size.ws, b->size.hs + y);
                for (unsigned x = 0; x < win_w; ++x)
                        putchar(' ');
        }

        // Draw all visible lines once
        for (size_t i = 0; i < win_h; ++i) {
                size_t idx = b->voff + i;
                if (idx >= b->lines.len)
                        break;
                drawln(b, idx);
        }

        // Place cursor
        unsigned screen_x = b->size.ws + (unsigned)(b->cx > b->hoff ? b->cx - b->hoff : 0);
        unsigned screen_y = b->size.hs + (unsigned)(b->cy - b->voff);
        gotoxy(screen_x, screen_y);
}
