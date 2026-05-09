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

#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

#include "array.h"

int   file_exists(const char *fp);
int   create_file(const char *fp, int force_overwrite);
int   is_dir(const char *path);
int   write_file(const char *fp, const char *content);
char *load_file(const char *path);

cstr_ar lsdir(const char *path);
cstr_ar walkdir(const char *path);

const char *gethome(void);
char       *get_realpath(const char *fp);
const char *get_basename(const char *name);

#endif // IO_H_INCLUDED
