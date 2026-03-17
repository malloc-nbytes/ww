#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include "lexer.h"

typedef enum {
        EXPR_KIND_IDENT,
        EXPR_KIND_INTLIT,
        EXPR_KIND_BINARY,
        EXPR_KIND_UNARY,
} expr_kind;

typedef struct {
        expr_kind kind;
} expr;

typedef struct {
        expr        base;
        const char *id;
} expr_ident;

typedef struct {
        expr base;
        int  i;
} expr_intlit;

typedef struct {
        expr       base;
        expr       *l;
        const char *op;
        expr       *r;
} expr_binary;

typedef struct {
        expr  base;
        char  op;
        expr *r;
} expr_unary;

expr *parse_expr(lexer *l);

#endif
