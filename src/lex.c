#include "lex.h"
#include "location.h"
#include "str.h"
#include "mem.h"

#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

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

static size_t
consume_while(const char *s, int (*pred)(int))
{
        size_t i = 0;
        while (s[i] && pred(s[i]))
                ++i;
        return i;
}

static int
isident(int c)
{
        return isalnum(c) || c == '_';
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
                char ch = src[i];
                if (isalpha(ch) || ch == '_') {
                        size_t len = consume_while(src+i, isident);
                        token *t = token_alloc(TK_IDENT, src+i, len, path, (unsigned)r, (unsigned)c);
                        array_append(l.tokens, t);
                        i += len;
                        c += len;
                } else if (isdigit(ch)) {
                        size_t len = consume_while(src+i, isdigit);
                        token *t = token_alloc(TK_INTLIT, src+i, len, path, (unsigned)r, (unsigned)c);
                        array_append(l.tokens, t);
                        i += len;
                        c += len;
                } else {
                        assert(0);
                }
        }

        return l;
}
