#include "visitor.h"

visitor
visitor_create(visit_expr_int_sig visit_expr_int,
               visit_expr_str_sig visit_expr_str,
               visit_expr_binary_sig visit_expr_binary,
               visit_stmt_expr_sig visit_stmt_expr)
{
        return (visitor) {
                .visit_expr_int    = visit_expr_int,
                .visit_expr_str    = visit_expr_str,
                .visit_expr_binary = visit_expr_binary,

                .visit_stmt_expr = visit_stmt_expr,
        };
}

void *
accept_expr_int(expr *e, visitor *v)
{
        if (v->visit_expr_int)
                v->visit_expr_int(v, (expr_int *)e);
        return NULL;
}

void *
accept_expr_str(expr *e, visitor *v)
{
        if (v->visit_expr_str)
                v->visit_expr_str(v, (expr_str *)e);
        return NULL;
}

void *
accept_expr_binary(expr *e, visitor *v)
{
        if (v->visit_expr_binary)
                v->visit_expr_binary(v, (expr_binary *)e);
        return NULL;
}

void *
accept_stmt_expr(stmt *s, visitor *v)
{
        if (v->visit_stmt_expr)
                v->visit_stmt_expr(v, (stmt_expr *)s);
        return NULL;
}

