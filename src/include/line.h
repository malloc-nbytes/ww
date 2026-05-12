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

#ifndef LINE_H_INCLUDED
#define LINE_H_INCLUDED

#include "str.h"
#include "array.h"

typedef struct {
        str txt;
} line;

ARRAY_DEFINE(line *, linep_ar);

line     *line_alloc(void);
line     *line_from(str s);
line     *line_create_nothing(void);
line     *line_from_cstr(const char *s);
void      line_append(line *ln, char ch);
linep_ar  lines_from(char *chars);
void      line_free(line *ln);

#endif // LINE_H_INCLUDED
