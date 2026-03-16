#include "parser.h"
#include "lexer.h"

#include <string.h>

static expr_ident *
expr_ident_alloc(const char *id)
{
        expr_ident *e;

        e            = (expr_ident *)malloc(sizeof(expr_ident));
        e->id        = strdup(id);
        e->base.kind = EXPR_KIND_IDENT;

        return e;
}

static expr_intlit *
expr_intlit_alloc(int i)
{
        expr_intlit *e;

        e            = (expr_intlit *)malloc(sizeof(expr_intlit));
        e->i         = i;
        e->base.kind = EXPR_KIND_INTLIT;

        return e;
}

static expr_binary *
expr_binary_alloc(expr *l, expr *r)
{
        expr_binary *e;

        e            = (expr_binary *)malloc(sizeof(expr_binary));
        e->l         = l;
        e->r         = r;
        e->base.kind = EXPR_KIND_BINARY;

        return e;
}

static expr_unary *
expr_unary_alloc(char op, expr *r)
{
        expr_unary *e;
        e            = (expr_unary *)malloc(sizeof(expr_unary));
        e->op        = op;
        e->r         = r;
        e->base.kind = EXPR_KIND_UNARY;

        return e;
}

expr *
parse_expr(lexer *l)
{
        (void)l;
        (void)expr_ident_alloc;
        (void)expr_intlit_alloc;
        (void)expr_binary_alloc;
        (void)expr_unary_alloc;

        return NULL;
}
