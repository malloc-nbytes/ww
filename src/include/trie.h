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

#ifndef TRIE_H_INCLUDED
#define TRIE_H_INCLUDED

#include "array.h"

#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__)
#  define WARN_UNUSED_RESULT  __attribute__((warn_unused_result))
#else
#  define WARN_UNUSED_RESULT
#endif

void *trie_alloc(void) WARN_UNUSED_RESULT;
int trie_insert(void *t, const char *word);
char **trie_get_completions(void       *t,
                            const char *prefix,
                            size_t      max_results,
                            size_t     *out_count);
int trie_empty(void *t);
void trie_destroy(void *t);

#endif // TRIE_H_INCLUDED

