#ifndef VISITOR_H_INCLUDED
#define VISITOR_H_INCLUDED

#include "grammar.h"

typedef void *(*visit_expr_int_sig)(visitor *, expr_int *);
typedef void *(*visit_expr_str_sig)(visitor *, expr_str *);
typedef void *(*visit_expr_binary_sig)(visitor *, expr_binary *);

typedef void *(*visit_stmt_expr_sig)(visitor *, stmt_expr *);

typedef struct visitor {
        void *ctx;
        visit_expr_int_sig visit_expr_int;
        visit_expr_str_sig visit_expr_str;
        visit_expr_binary_sig visit_expr_binary;

        visit_stmt_expr_sig visit_stmt_expr;
} visitor;

visitor visitor_create(visit_expr_int_sig visit_expr_int,
                       visit_expr_str_sig visit_expr_str,
                       visit_expr_binary_sig visit_expr_binary,
                       visit_stmt_expr_sig visit_stmt_expr);

void *accept_expr_int(expr *e, visitor *v);
void *accept_expr_str(expr *e, visitor *v);
void *accept_expr_binary(expr *e, visitor *v);

void *accept_stmt_expr(stmt *s, visitor *v);

#endif // VISITOR_H_INCLUDED