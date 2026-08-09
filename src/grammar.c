#include "grammar.h"
#include "visitor.h"

stmt_expr *
stmt_expr_alloc(expr *e)
{
        stmt_expr *s;

        s              = (stmt_expr *)malloc(sizeof(*s));
        s->e           = e;
        s->base.loc    = e->loc;
        s->base.k      = STMT_KIND_EXPR;
        s->base.accept = accept_stmt_expr;

        return s;
}
