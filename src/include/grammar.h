#ifndef GRAMMAR_H_INCLUDED
#define GRAMMAR_H_INCLUDED

#include "lex.h"
#include "location.h"
#include "array.h"

typedef struct visitor visitor;

typedef enum {
        EXPR_KIND_INT = 0,
        EXPR_KIND_STR,
        EXPR_KIND_BINARY,
} expr_kind;

typedef enum {
        STMT_KIND_EXPR = 0,
} stmt_kind;

typedef struct expr {
        expr_kind k;
        location loc;
        void *(*accept)(struct expr *, visitor *);
} expr;

typedef struct stmt {
        stmt_kind k;
        location loc;
        void *(*accept)(struct stmt *, visitor *);
} stmt;

ARRAY_DEFINE(stmt *, stmtp_ar);

typedef struct {
        expr base;
        token *i;
} expr_int;

typedef struct {
        expr base;
        token *s;
} expr_str;

typedef struct {
        expr base;
        expr *lhs;
        token *op;
        expr *rhs;
} expr_binary;

typedef struct {
        stmt base;
        expr *e;
} stmt_expr;

stmt_expr *stmt_expr_alloc(expr *e);

#endif // GRAMMAR_H_INCLUDED
