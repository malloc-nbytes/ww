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

#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED

#include <stddef.h>
#include <stdlib.h>

#define MAP_DEFAULT_CAPACITY 2048

#define MAP_DEFINE(ktype, vtype, mapname) \
        typedef unsigned (*mapname##_hash_sig)(ktype *); \
        typedef int      (*mapname##_cmp_sig)(ktype *, ktype *); \
        \
        typedef struct __mapname##_node { \
                ktype k; \
                vtype v; \
                struct __mapname##_node *n; \
        } __##mapname##_node; \
        \
        typedef struct { \
                struct { \
                        __##mapname##_node **data; \
                        size_t len; \
                        size_t cap; \
                        size_t sz; \
                } tbl; \
                mapname##_hash_sig hash; \
                mapname##_cmp_sig cmp; \
        } mapname; \
        \
        mapname mapname##_create(mapname##_hash_sig hash, mapname##_cmp_sig cmp); \
        void mapname##_destroy(mapname *map); \
        void mapname##_insert(mapname *map, ktype k, vtype v); \
        int mapname##_contains(mapname *map, ktype k); \
        vtype *mapname##_get(mapname *map, ktype k)

#define MAP_IMPL(ktype, vtype, mapname) \
        mapname \
        mapname##_create(mapname##_hash_sig hash, \
                         mapname##_cmp_sig cmp) \
        { \
                __##mapname##_node **data \
                        = (__##mapname##_node **)calloc(MAP_DEFAULT_CAPACITY, sizeof(__##mapname##_node *)); \
                return (mapname) { \
                        .tbl = { \
                                .data = data, \
                                .len = 0, \
                                .cap = MAP_DEFAULT_CAPACITY, \
                                .sz = 0, \
                        }, \
                        .hash = hash, \
                        .cmp = cmp, \
                }; \
        } \
        void \
        mapname##_insert(mapname *map, ktype k, vtype v) \
        { \
                unsigned idx = map->hash(&k) % map->tbl.cap;  \
                __##mapname##_node *it = map->tbl.data[idx]; \
                __##mapname##_node *prev = NULL; \
                while (it) { \
                        if (!map->cmp(&it->k, &k)) {   \
                                it->v = v; \
                                return; \
                        } \
                        prev = it; \
                        it = it->n; \
                } \
                it = (__##mapname##_node *)malloc(sizeof(__##mapname##_node)); \
                it->k = k; \
                it->v = v; \
                it->n = NULL; \
                if (prev) { \
                        prev->n = it; \
                } else { \
                        map->tbl.data[idx] = it; \
                        ++map->tbl.len; \
                } \
                ++map->tbl.sz; \
        } \
        \
        int \
        mapname##_contains(mapname *map, ktype k) \
        { \
                return mapname##_get(map, k) != NULL; \
        } \
        \
        vtype * \
        mapname##_get(mapname *map, ktype k) \
        { \
                unsigned idx = map->hash(&k) % map->tbl.cap; \
                __##mapname##_node *it = map->tbl.data[idx]; \
                while (it) { \
                        if (!map->cmp(&it->k, &k)) {  \
                                return &it->v; \
                        } \
                        it = it->n; \
                } \
                return NULL; \
        }

#endif // MAP_H_INCLUDED
