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
void trie_destroy(void *t);

#endif // TRIE_H_INCLUDED

