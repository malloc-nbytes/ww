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

#ifndef MINIBUFFER_H_INCLUDED
#define MINIBUFFER_H_INCLUDED

#include "str.h"
#include "ww.h"
#include "array.h"

char *
minibuffer_input(ww         *ed,
                 const char *label,
                 const char *autofill,
                 cstr_ar     items);

#endif // MINIBUFFER_H_INCLUDED
