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

#ifndef PAIR_H_INCLUDED
#define PAIR_H_INCLUDED

#define PAIR_DEFINE(ty0, ty1, name) \
        typedef struct { \
                ty0 l; \
                ty1 r; \
        } name; \
        \
        name name##_create(ty0 l, ty1 r); \
        ty0 name##_getl(name *p); \
        ty1 name##_getr(name *p)

#define PAIR_IMPL(ty0, ty1, name) \
        name \
        name##_create(ty0 l, ty1 r) \
        { \
                return (name) { .l = l, .r = r, }; \
        } \
        \
        ty0 \
        name##_getl(name *p) \
        { \
                return p->l; \
        } \
        \
        ty1 \
        name##_getr(name *p) \
        { \
                return p->r; \
        }

#endif // PAIR_H_INCLUDED
