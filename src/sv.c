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

#include "sv.h"
#include "error.h"

#include <assert.h>
#include <string.h>
#include <errno.h>

sv
sv_from(const char *s, ssize_t len)
{
        return (sv) {
                .s = s,
                .len = len == -1 ? strlen(s) : (size_t)len,
        };
}

const char *
sv_cstr(sv view)
{
#define BUF_CAP 1024
        assert(view.len <= BUF_CAP);

        static char buf[BUF_CAP+1];

        memset(buf, 0, sizeof(buf));
        if (!strncpy(buf, view.s, view.len))
                fatal("failed to copy string: %s", strerror(errno));

        return buf;
#undef BUF_CAP
}
