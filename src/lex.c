#include "lex.h"
#include "location.h"
#include "str.h"
#include "mem.h"

#include <assert.h>
#include <stdio.h>
#include <stddef.h>

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
        assert(0);
}
