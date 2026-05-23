/*
 * ww: a simple editor
 * Copyright (C) 2026 malloc-nbytes
 * Contact: zdhdev@yahoo.com

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#include "array.h"
#include "line.h"
#include "str.h"
#include "set.h"

#define BUFFER_BUILTIN_COMPILE "ww-compile"
#define BUFFER_BUILTIN_HELP    "ww-help"
#define BUFFER_BUILTIN_MAN     "ww-man"

extern char_ar g_cpy_buf;

typedef struct ww ww;

typedef enum {
        BA_NONE = 0,
        BA_NOP,
        BA_REDRAW,
        BA_XY,
        BA_REQ_EXIT,
        BA_REQ_FINDFILE,
        BA_REQ_SWITCHBUFFER,
        BA_REQ_SPLITVER,
        BA_REQ_JMPBUF,
        BA_REQ_METAX,
        BA_REQ_MAXIMIZEMON,
        BA_REQ_COMPILE,
        BA_REQ_RECOMPILE,
        BA_REQ_CLOSE_BUILTIN,
        BA_REQ_SPLITHOR,
        BA_REQ_KILLBUF,
        BA_REQ_SWITCHCOMPL,
        BA_REQ_ERRJMP,
        BA_REQ_NEXTERROR,
        BA_REQ_PREVERROR,
} buffer_action;

typedef enum {
        BS_NORMAL = 0,
        BS_SEARCH,
        BS_SELECTION,
        BS_AUTO,
} buffer_state;

typedef struct {
        str name; // name of buffer, can be same as path
                  // if buffer by the same name exists
        str path; // path to the file we are editing
        int builtin; // is this a builtin buffer
        struct {
                unsigned w;  // width
                unsigned h;  // height
                unsigned ws; // width start
                unsigned hs; // height start
        } size;
        linep_ar     lines;    // lines in the buffer
        unsigned     cx;       // cursor x (logical & visual)
        unsigned     cy;       // cursor y (visual)
        unsigned     wish_col; // wished column to jump to
        size_t       al;       // active line
        size_t       voff;     // vertical scroll offset
        size_t       hoff;     // horizontal scroll offset
        buffer_state state;    // current state
        int          saved;    // is the buffer saved
        unsigned     sx;       // buffer selection x
        unsigned     sy;       // buffer selection y
        int          writable; // is buffer writable
        str          last_search; // last search query
        ww          *parent;      // parent editor
        int          last_tab;    // was the last character a tab
        void        *ac;          // autocomplete
        size_t       ac_cycle;    // current autocomplete cycle
        cstr_set     found_words; // found words for autocomplete
} buffer;

ARRAY_DEFINE(buffer *, bufferp_ar);

void    init_buffer_translation_unit(void);
buffer *buffer_from(str       name,
                    str       path,
                    unsigned  w,
                    unsigned  h,
                    unsigned  ws,
                    unsigned  hs,
                    linep_ar  lns,
                    ww       *parent);

void           buffer_draw(const buffer *b);
void           buffer_drawxy(const buffer *b);
buffer_action  buffer_process(buffer *b);
void           buffer_make_readonly(buffer *b);
buffer_action  buffer_save(buffer *b);
buffer_action  buffer_adjust_scroll(buffer *b);
void           buffer_make_builtin(buffer *b);
buffer        *ww_helpbuf_alloc(unsigned w, unsigned h, unsigned ws, unsigned hs, ww *parent);
void           buffer_free(buffer *b);
void           buffer_jump_to_verts(buffer *b, size_t x, size_t y);
buffer_action  buffer_center_view(buffer *b);

#endif // BUFFER_H_INCLUDED
