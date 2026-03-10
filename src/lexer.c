#include "lexer.h"
#include "io.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static token *
token_alloc(const char *st,
            size_t      len,
            token_kind  k,
            const char *fp,
            size_t      r,
            size_t      c)
{
        token *t;
        tloc   loc;
        char   buf[1024];

        t = (token *)malloc(sizeof(token));

        loc = (tloc) {
                .fp = fp,
                .r  = r,
                .c  = c,
        };

        snprintf(buf, len, "%s", st);

        t->loc = loc;
        t->lx  = str_from(buf);
        t->k   = k;

        return t;
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

lexer_cfg
lexer_cfg_from_filext(const char *ext)
{
        (void)ext;
        assert(0 && "unimplemented");
}

lexer
lex_file(lexer_cfg cfg)
{
        lexer       l;
        size_t      r;
        size_t      c;
        size_t      i;
        const char *src;
        size_t      src_n;

        l = (lexer) {
                .root = NULL,
                .hd   = NULL,
                .tl   = NULL,
                .err  = NULL,
                .cfg  = cfg,
        };

        r     = 1;
        c     = 1;
        i     = 0;
        src   = cfg.src;
        src_n = strlen(src);

        while (i < src_n) {
                char ch = src[i];

                if (memcmp(src+i, cfg.mlop, strlen(cfg.mlop))) {
                        assert(0);
                } else if (memcmp(src+i, cfg.mlcl, strlen(cfg.mlcl))) {
                        assert(0);
                } else if (memcmp(src+i, cfg.sl, strlen(cfg.sl))) {
                        assert(0);
                } else if (src[i] == '\n') {
                        assert(0);
                } else if (src[i] == '\t') {
                        assert(0);
                } else if (src[i] == ' ') {
                        assert(0);
                } else if (isalpha(ch) || ch == '_') {
                        assert(0);
                } else if (isdigit(ch)) {
                        assert(0);
                } else {
                        ;
                }
        }

        return l;
}

void
lexer_free(lexer *l)
{
        (void)l;
        assert(0);
}
