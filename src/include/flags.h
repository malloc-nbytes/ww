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

#ifndef FLAGS_H_INCLUDED
#define FLAGS_H_INCLUDED

typedef enum {
        FK_TABMODE        = 1 << 0,
        FK_NOAUTOBRACKET  = 1 << 1,
        FK_SHOWTRAILS     = 1 << 2,
} flag_kind;

#define FLAG_1HY_HELP 'h'
#define FLAG_2HY_HELP "help"

#define FLAG_2HY_COPYING "copying"

#define FLAG_2HY_CREATECONFIG "create-config"

#define FLAG_1HY_VERSION 'v'
#define FLAG_2HY_VERSION "version"

#endif // FLAGS_H_INCLUDED
