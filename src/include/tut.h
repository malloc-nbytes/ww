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

#ifndef TUT_H_INCLUDED
#define TUT_H_INCLUDED

#include "buffer.h"
#include "ww.h"

#define TUT_CH1_NAME "Basic Navigation"
#define TUT_CH2_NAME "Basic Editing"
#define TUT_CH3_NAME "Status Bar"
#define TUT_CH4_NAME "Autocomplete"

buffer *tut_alloc(ww *ed, const char *chapter);

#endif // TUT_H_INCLUDED
