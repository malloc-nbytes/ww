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

#ifndef SV_H_INCLUDED
#define SV_H_INCLUDED

#include <stddef.h>
#include <stdlib.h>

typedef struct {
        const char *s;
        size_t      len;
} sv;

// len=-1 for taking the length until '\0'.
sv sv_from(const char *s, ssize_t len);

const char *sv_cstr(sv sv);

#endif
