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

#include "colors.h"

#include <stddef.h>
#include <stdint.h>
#include <termios.h>

typedef struct qcl_config qcl_config;

struct {
        struct {
                size_t         w;
                size_t         h;
                struct termios termios;
        } term;
        struct {
                char *compile;
                int   space_amt;
                char *artwork;
                const char *to_clipboard;
                char *selection_highlight;
                char *search_highlight;
                char *search_highlight_exact;
                char *menu_highlight;
                int   disable_quit_keybind;
        } runtime;
        uint32_t    flags;
        qcl_config *config;
} glconf = {
        .term = {
                .w       = 0,
                .h       = 0,
                .termios = {0},
        },
        .runtime = {
                .compile                = NULL,
                .space_amt              = 8,
                .artwork                = "ww1",
                .to_clipboard           = "echo -E '%%s' | xclip -selection clipboard",
                .selection_highlight    = INVERT,
                .search_highlight       = INVERT DIM YELLOW,
                .search_highlight_exact = INVERT BOLD ORANGE,
                .menu_highlight         = YELLOW BOLD INVERT,
                .disable_quit_keybind   = 0,
        },
        .flags  = 0x0000,
        .config = NULL,
};
