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

static int
iskwd(const char *st, size_t len)
{
        static char *kwds[] = LEXER_C_KWDS;
        for (size_t i = 0; i < sizeof(kwds)/sizeof(*kwds); ++i) {
                if (memcmp(kwds[i], st, len) == 0)
                        return 1;
        }
        return 0;
}

static size_t
consume_while(const char *st,
              int (*pred)(int))
{
        size_t i = 0;

        while (st[i] && pred(st[i]))
                ++i;

        return i;
}

static int isident(int ch) { return isalnum(ch) || ch == '_'; }

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

                if (memcmp(src+i, cfg.mlop, strlen(cfg.mlop)) == 0) {
                        assert(0);
                } else if (memcmp(src+i, cfg.mlcl, strlen(cfg.mlcl)) == 0) {
                        assert(0);
                } else if (memcmp(src+i, cfg.sl, strlen(cfg.sl)) == 0) {
                        assert(0);
                } else if (src[i] == '\n') {
                        if (cfg.bits & LEXERCFG_TRACK_NEWLINES)
                                append(&l, token_alloc(src+i, 1, TK_NL, cfg.fp, r, c));
                        ++i;
                        ++r;
                        c = 1;
                } else if (src[i] == '\t') {
                        if (cfg.bits & LEXERCFG_TRACK_TABS)
                                append(&l, token_alloc(src+i, 1, TK_TAB, cfg.fp, r, c));
                        ++i;
                        ++c;
                } else if (src[i] == ' ') {
                        if (cfg.bits & LEXERCFG_TRACK_SPACES)
                                append(&l, token_alloc(src+i, 1, TK_SPC, cfg.fp, r, c));
                        ++i;
                        ++c;
                } else if (isalpha(ch) || ch == '_') {
                        size_t len = consume_while(src+i, isident);
                        append(&l, token_alloc(src+i, len, iskwd(src+i, len) ? TK_KW : TK_ID, cfg.fp, r, c));
                        i += len;
                        c += len;
                } else if (isdigit(ch)) {
                        size_t len = consume_while(src+i, isdigit);
                        append(&l, token_alloc(src+i, len, TK_INTL, cfg.fp, r, c));
                        i += len;
                        c += len;
                } else {
                        assert(0);
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
