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
                char qc = tolower(query[qi]);
                int found = 0;

                while (wi < wlen) {
                        char wc = tolower(word[wi]);

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

cstr_array
fuzzy_find(cstr_array  words,
           const char *query)
{
        cstr_array result = dyn_array_empty(cstr_array);

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
                dyn_array_append(result, matches[i].word);

        free(matches);
        return result;
}
