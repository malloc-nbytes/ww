#include "lex.h"
#include "location.h"
#include "str.h"
#include "mem.h"

#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

static token *
token_alloc(token_kind kind,
            const char *st,
            size_t     len,
            const char *path,
            unsigned    r,
            unsigned    c)
{
        token *t;
        char buf[1024] = {0};

        (void)snprintf(buf, len, "%s", st);

        t       = (token *)alloc(sizeof(token));
        t->kind = kind;
        t->lx   = str_from(buf);
        t->loc  = location_from(path, r, c);

        return t;
}

lexer
lex(lexer_cfg cfg)
{
        lexer       l;
        const char *src;
        const char *path;
        size_t      r;
        size_t      c;
        size_t      i;
        size_t      n;

        l = (lexer) {
                .tokens = array_empty(tokenp_ar),
                .pos    = 0,
                .cfg    = cfg,
        };

        src  = cfg.src;
        path = cfg.path;
        r    = 1;
        c    = 1;
        i    = 0;
        n    = strlen(src);

        while (i < n) {
                assert(0);
        }
}
