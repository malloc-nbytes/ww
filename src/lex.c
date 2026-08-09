#include "lex.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

MAP_IMPL(const char *, token_kind, opmap);

static token *
token_alloc(token_kind  k,
            const char *st,
            size_t      len,
            size_t      r,
            size_t      c,
            const char *path)
{
        token *t;

        t      = (token *)malloc(sizeof(*t));
        t->k   = k;
        t->lx  = sv_from(st, (ssize_t)len);
        t->loc = location_from(r, c, path);
        t->n   = NULL;

        return t;
}

static void
append(lexer *l, token *t)
{
        array_append(l->tokens, t);
        if (l->tokens.len > 1)
                l->tokens.data[l->tokens.len-2]->n = l->tokens.data[l->tokens.len-1];
}

static size_t
consume_while(const char *s, int(*pred)(int))
{
        size_t i;
        for (i = 0; s[i] && pred(s[i]); ++i);
        return i;
}

static int
not_double_quote(int c)
{
        return c != '"';
}

static int
isident(int c)
{
        return c == '_' || isalnum(c);
}

static int
isop(int c)
{
        return !isident(c) && not_double_quote(c) && !isspace(c);
}

static token_kind *
determine_op(const char *s, size_t *len, opmap m)
{
        token_kind *k;
        char buf[32] = {0};

        k = NULL;

        (void)memcpy(buf, s, *len);

        while (*len > 0) {
                if (opmap_contains(&m, buf))
                        return opmap_get(&m, buf);
                buf[--(*len)] = 0;
        }

        return k;
}

static const char *
token_kind_ccstr(token_kind k)
{
        switch (k) {
        case TOKEN_KIND_EOF:            return "EOF";
        case TOKEN_KIND_LCURLY:         return "TOKEN_KIND_LCURLY";
        case TOKEN_KIND_RCURLY:         return "TOKEN_KIND_RCURLY";
        case TOKEN_KIND_LSQR:           return "TOKEN_KIND_LSQR";
        case TOKEN_KIND_RSQR:           return "TOKEN_KIND_RSQR";
        case TOKEN_KIND_COLON:          return "TOKEN_KIND_COLON";
        case TOKEN_KIND_COMMA:          return "TOKEN_KIND_COMMA";
        case TOKEN_KIND_STRING_LITERAL: return "TOKEN_KIND_STRING_LITERAL";
        case TOKEN_KIND_NUMBER_LITERAL: return "TOKEN_KIND_NUMBER_LITERAL";
        case TOKEN_KIND_KEYWORD:        return "TOKEN_KIND_KEYWORD";
        default: break;
        }
        return "<unknown>";
}

void
lexer_dump(const lexer *l)
{
        for (size_t i = 0; i < l->tokens.len; ++i) {
                const token *t = l->tokens.data[i];
                printf("<lx=%s, k=%s, ", sv_view(t->lx), token_kind_ccstr(t->k));
                printf("loc=%s, n=%p>", loc_fmt_cstr(t->loc), t->n);
        }
}

token *
lexer_peek(const lexer *l)
{
        if (l->cursor >= l->tokens.len)
                return NULL;
        return l->tokens.data[l->cursor];
}

lexer
lex_file(lexer_cfg cfg)
{
        lexer l;
        size_t r, c;

        l = (lexer) {
                .cfg = cfg,
                .tokens = array_empty(tokenp_ar),
                .cursor = 0,
        };

        r = 1, c = 1;

        for (size_t i = 0; i < l.cfg.lns.len; ++i) {
                const line *ln = l.cfg.lns.data[i];
                const str  *s  = &ln->txt;

                for (size_t j = 0; j < s->len; ++j) {
                        const char *src = s->chars;
                        char        ch  = src[j];
                        if (ch == '\n') {
                                c = 1;
                                ++r;
                                ++i;
                        } else if (isspace(ch)) {
                                ++c;
                                ++i;
                        } else if (isalpha(ch) || ch == '_') {
                                assert(0 && "identifiers unimplemented");
                        } else if (ch == '"') {
                                size_t len = consume_while(src+i+1, not_double_quote);
                                token *t = token_alloc(TOKEN_KIND_STRING_LITERAL,
                                                       src+i+1, len, r, c,
                                                       l.cfg.path);
                                append(&l, t);
                                i += len+2;
                                c += len+2;
                        } else if (ch == '\'') {
                                assert(0 && "single quotes unimplemented");
                        } else if (isdigit(ch)) {
                                size_t len = consume_while(src+i, isdigit);
                                token *t = token_alloc(TOKEN_KIND_NUMBER_LITERAL,
                                                       src+i, len, r, c,
                                                       l.cfg.path);
                                i += len;
                                c += len;
                                append(&l, t);
                        } else {
                                size_t len = consume_while(src+i, isop);
                                token_kind *k = determine_op(src+i, &len, l.cfg.ops);
                                if (!k) {
                                        assert(0 && "unhandled operator");
                                }
                                token *t = token_alloc(*k, src+i, len, r, c, l.cfg.path);
                                append(&l, t);
                                i += len;
                                c += len;
                        }
                }
        }

        return l;
}
