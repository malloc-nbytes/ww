#include "lexer.h"
#include "io.h"
#include "map.h"
#include "error.h"
#include "str.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

MAP_TYPE(char *, token_kind, opmap)

static void
opmap_init(opmap *m)
{
        opmap_insert(m, "(", TK_LPAR);
        opmap_insert(m, ")", TK_RPAR);
        opmap_insert(m, ";", TK_SEMI);
        opmap_insert(m, "=", TK_EQ);
        opmap_insert(m, "==", TK_EQEQ);
        opmap_insert(m, "+", TK_PLUS);
        opmap_insert(m, "-", TK_MINUS);
        opmap_insert(m, "/", TK_FORSL);
        opmap_insert(m, "\\", TK_BAKSL);
        opmap_insert(m, "#", TK_HASH);
        opmap_insert(m, ":", TK_COLON);
        opmap_insert(m, "!", TK_BANG);
        opmap_insert(m, ">", TK_GT);
        opmap_insert(m, "<", TK_LT);
        opmap_insert(m, ">=", TK_GTE);
        opmap_insert(m, "<=", TK_LTE);
        opmap_insert(m, "!=", TK_BANGEQ);
        opmap_insert(m, "{", TK_LCUR);
        opmap_insert(m, "}", TK_RCUR);
        opmap_insert(m, "[", TK_LSQR);
        opmap_insert(m, "]", TK_RSQR);
        opmap_insert(m, ".", TK_PERIOD);
        opmap_insert(m, ",", TK_COMMA);
        opmap_insert(m, "*", TK_AST);
        opmap_insert(m, "&", TK_AMP);
        opmap_insert(m, "?", TK_QUEST);
        opmap_insert(m, "%", TK_PERC);
        opmap_insert(m, "|", TK_PIPE);
        opmap_insert(m, "+=", TK_PLUSEQ);
        opmap_insert(m, "-=", TK_MINUSEQ);
        opmap_insert(m, "*=", TK_ASTEQ);
        opmap_insert(m, "/=", TK_FORSLEQ);
        opmap_insert(m, "%=", TK_PERCEQ);
        opmap_insert(m, "^", TK_UPTICK);
        opmap_insert(m, "||", TK_PIPEPIPE);
        opmap_insert(m, "&&", TK_AMPAMP);
        opmap_insert(m, "&=", TK_AMPEQ);
        opmap_insert(m, "|=", TK_PIPEEQ);
        opmap_insert(m, "^=", TK_UPTICKEQ);
}

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

        snprintf(buf, len+1, "%s", st);

        t->loc = loc;
        t->lx  = str_from(buf);
        t->k   = k;
        t->n   = NULL;

        return t;
}

static void
token_free(token *t)
{
        str_destroy(&t->lx);
        free(t);
}

static const char *
tloc_cstr(tloc loc)
{
        static char buf[256];
        sprintf(buf, "%s:%zu:%zu", loc.fp, loc.r, loc.c);
        return buf;
}

token *
lexer_hd(lexer *l)
{
        return l->hd;
}

static void
lexer_append(lexer *l, token *t)
{
        if (!l->hd && !l->tl) {
                l->hd = l->tl = t;
                l->root = t;
        } else {
                token *tmp = l->tl;
                l->tl = t;
                tmp->n = t;
        }
}

const token *
lexer_peek(lexer *l, size_t k)
{
        token *it;

        it = lexer_hd(l);

        for (size_t i = 0; i < k; ++i) {
                if (!it)
                        return NULL;
                it = it->n;
        }

        return it;
}

token *
lexer_next(lexer *l)
{
        if (!lexer_hd(l))
                return NULL;

        token *t;

        t     = l->hd;
        l->hd = l->hd->n;

        return t;
}

static void
lexer_dump(const lexer *l)
{
        token *it;

        it = l->hd;

        while (it) {
                printf("{ %s (%d): %s }\n",
                        str_cstr(&it->lx), (int)it->k, tloc_cstr(it->loc));
                it = it->n;
        }
}

lexer_cfg
lexer_cfg_from_filext(const char *ext)
{
        (void)ext;
        assert(0 && "unimplemented");
}

static int
iskwd(const char  *st,
      size_t       len,
      const char **kwds)
{
        if (!kwds)
                return 0;

        for (size_t i = 0; kwds[i]; ++i) {
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
static int noquote(int ch) { return ch != '"'; }
static int isop(int ch)
{
        return !isalnum(ch)
                && ch != '_'
                && ch != ' '
                && ch != '\t'
                && ch != '\n'
                && ch != '\r';
}

static token_kind *
determine_op(opmap *m, const char *s, size_t *len)
{
        assert(*len < 256);
        char buf[256] = {0};

        while (*len > 0) {
                memset(buf, 0, 256);
                strncpy(buf, s, *len);
                if (opmap_contains(m, buf)) {
                        return opmap_get(m, buf);
                }
                --(*len);
        }

        return NULL;
}

static unsigned
opmap_hash(char **s)
{
        return **s;
}

static int
opmap_cmp(char **s0, char **s1)
{
        return strcmp(*s0, *s1);
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
        opmap       ops;

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
        ops   = opmap_create(opmap_hash, opmap_cmp);

        opmap_init(&ops);

        while (i < src_n) {
                char ch = src[i];

                if (cfg.mlop && memcmp(src+i, cfg.mlop, strlen(cfg.mlop)) == 0) {
                        assert(0);
                } else if (cfg.mlcl && memcmp(src+i, cfg.mlcl, strlen(cfg.mlcl)) == 0) {
                        assert(0);
                } else if (cfg.sl && memcmp(src+i, cfg.sl, strlen(cfg.sl)) == 0) {
                        while (i < src_n && src[i] != '\n')
                                ++i, ++c;
                        ++i;
                        ++r;
                        c = 1;
                } else if (src[i] == '\n') {
                        if (cfg.bits & LEXERCFG_TRACK_NEWLINES)
                                lexer_append(&l, token_alloc(src+i, 1, TK_NL, cfg.fp, r, c));
                        ++i;
                        ++r;
                        c = 1;
                } else if (src[i] == '\t') {
                        if (cfg.bits & LEXERCFG_TRACK_TABS)
                                lexer_append(&l, token_alloc(src+i, 1, TK_TAB, cfg.fp, r, c));
                        ++i;
                        ++c;
                } else if (src[i] == ' ') {
                        if (cfg.bits & LEXERCFG_TRACK_SPACES)
                                lexer_append(&l, token_alloc(src+i, 1, TK_SPC, cfg.fp, r, c));
                        ++i;
                        ++c;
                } else if (isalpha(ch) || ch == '_') {
                        size_t len = consume_while(src+i, isident);
                        lexer_append(&l, token_alloc(src+i, len, iskwd(src+i, len, l.cfg.kwds) ? TK_KW : TK_ID, cfg.fp, r, c));
                        i += len;
                        c += len;
                } else if (isdigit(ch)) {
                        size_t len = consume_while(src+i, isdigit);
                        lexer_append(&l, token_alloc(src+i, len, TK_INTL, cfg.fp, r, c));
                        i += len;
                        c += len;
                } else if (src[i] == '"' || src[i] == '\'') {
                        ++i, ++c;
                        size_t len = consume_while(src+i, noquote);
                        lexer_append(&l, token_alloc(src+i, len, TK_STRL, cfg.fp, r, c));
                        i += len+1;
                        c += len+1;
                } else {
                        size_t      len;
                        token_kind *k;

                        len = consume_while(src+i, isop);
                        if (!(k = determine_op(&ops, src+i, &len)))
                                fatal("unknown symbol at %c", src[i]);

                        lexer_append(&l, token_alloc(src+i, len, *k, cfg.fp, r, c));

                        i += len;
                        c += len;
                }
        }

        return l;
}

void
lexer_destroy(lexer *l)
{
        while (l->hd) {
                token *tmp = l->hd->n;
                token_free(l->hd);
                l->hd = tmp;
        }
}
