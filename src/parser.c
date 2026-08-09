#include "parser.h"

#include <assert.h>

#define PEEK(p) lexer_peek((p).l) && lexer_peek((p).l)
#define INBOUNDS(p) ((p).l->cursor < (p).l->tokens.len)

static expr *
parse_expr(parser *p)
{
        assert(0 && p && "unimplemented");
}

static stmt_expr *
parse_stmt_expr(parser *p)
{
        expr *e;

        if (!(e = parse_expr(p)))
                return NULL;

        return stmt_expr_alloc(e);
}

static stmt *
parse_stmt(parser *p)
{
        return (stmt *)parse_stmt_expr(p);
}

parser
parse_ast(lexer *l)
{
        parser p;

        p = (parser) {
                .l = l,
                .stmts = array_empty(stmtp_ar),
        };

        while (PEEK(p)->k != TOKEN_KIND_EOF) {
                stmt *s;
                if (!(s = parse_stmt(&p))) {
                        assert(0 && "parsing error");
                }
                array_append(p.stmts, s);
        }

        return p;
}
