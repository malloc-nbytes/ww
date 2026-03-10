#include "lexer.h"
#include "io.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static token *
token_alloc(const char *st,
            size_t      len,
            token_kind  k,
            const char *fp,
            size_t      r,
            size_t      c)
{
        assert(0);
}

static void
append(lexer *l, token *t)
{
        (void)l;
        (void)t;
        assert(0);
}

void
lexer_dump(const lexer *l)
{
        (void)l;
        assert(0);
}

lexer
lex_file(const char *fp)
{
        lexer       l;
        const char *src;
        size_t      r;
        size_t      c;
        size_t      i;

        l = (lexer) {
                .root = NULL,
                .hd   = NULL,
                .tl   = NULL,
                .err  = NULL,
        };

        if (!(src = load_file(fp))) {
                char msg[512] = {0};
                sprintf(msg, "could not load file `%s'", fp);
                l.err = strdup(msg);
                return l;
        }

        r = 1;
        c = 1;
        i = 0;

        while (src[i]) {
                assert(0);
        }

        return l;
}

void
lexer_free(lexer *l)
{
        (void)l;
        assert(0);
}

