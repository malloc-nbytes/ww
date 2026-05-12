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

#include "line.h"
#include "str.h"
#include "mem.h"

line *
line_alloc(void)
{
        line *l;

        l = (line *)alloc(sizeof(line));

        l->txt = str_from("\n");

        return l;
}

line *
line_create_nothing(void)
{
        line *l;

        l      = (line *)alloc(sizeof(line));
        l->txt = str_create();

        return l;
}

line *
line_from(str s)
{
        line *l;

        l      = (line *)alloc(sizeof(line));
        l->txt = s;

        return l;
}

line *
line_from_cstr(const char *s)
{
        line *l;

        l      = (line *)alloc(sizeof(line));
        l->txt = str_from(s);

        return l;
}

void
line_append(line *ln, char ch)
{
        str_append(&ln->txt, ch);
}

linep_ar
lines_from(char *chars)
{
        linep_ar ar;
        str      buf;

        ar  = array_empty(linep_ar);
        buf = str_create();

        for (size_t i = 0; chars[i]; ++i) {
                str_append(&buf, chars[i]);
                if (chars[i] == '\n') {
                        array_append(ar, line_from(str_from(buf.chars)));
                        str_clear(&buf);
                }
        }

        if (buf.len > 0) {
                str_append(&buf, '\n');
                array_append(ar, line_from(str_dup(buf)));
        }

        str_destroy(&buf);

        return ar;
}

void
line_free(line *ln)
{
        str_destroy(&ln->txt);
        free(ln);
}
