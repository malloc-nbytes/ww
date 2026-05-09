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

#include "fuzzy.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct {
        char *word;
        int score;
} match_t;

static int
is_word_boundary(const char *s, size_t i)
{
        if (i == 0)
                return 1;
        char prev = s[i - 1];
        return prev == '_' || prev == '-' || prev == ' ' || prev == '/';
}

static int
fuzzy_score(const char *word,
            const char *query)
{
        size_t wlen = strlen(word);
        size_t qlen = strlen(query);

        if (qlen == 0)
                return 0;

        size_t wi = 0;
        int score = 0;
        int consecutive = 0;

        for (size_t qi = 0; qi < qlen; ++qi) {
                char qc = (char)tolower(query[qi]);
                int found = 0;

                while (wi < wlen) {
                        char wc = (char)tolower(word[wi]);

                        if (wc == qc) {
                                found = 1;

                                score += 10;

                                if (consecutive)
                                        score += 15;

                                if (is_word_boundary(word, wi))
                                        score += 20;

                                consecutive = 1;
                                wi++;
                                break;
                        }

                        consecutive = 0;
                        score -= 1; // gap penalty
                        wi++;
                }

                if (!found)
                        return -1; // not a subsequence
        }

        return score;
}

static int
cmp_match(const void *a,
          const void *b)
{
        const match_t *ma = a;
        const match_t *mb = b;
        return mb->score - ma->score; // descending
}

cstr_ar
fuzzy_find(cstr_ar     words,
           const char *query)
{
        cstr_ar result = array_empty(cstr_ar);

        if (words.len == 0)
                return result;

        // Temporary match storage
        match_t *matches = malloc(sizeof(match_t) * words.len);
        size_t match_count = 0;

        for (size_t i = 0; i < words.len; ++i) {
                int score = fuzzy_score(words.data[i], query);
                if (score >= 0) {
                        matches[match_count++] = (match_t){
                                .word = words.data[i],
                                .score = score
                        };
                }
        }

        // Sort matches by score
        qsort(matches, match_count, sizeof(match_t), cmp_match);

        // Build result array
        for (size_t i = 0; i < match_count; ++i)
                array_append(result, matches[i].word);

        free(matches);
        return result;
}
