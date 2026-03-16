#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

typedef enum {
        EXPR_KIND_IDENT,
        EXPR_KIND_INTLIT,
        EXPR_KIND_BINARY,
        EXPR_KIND_UNARY,
        EXPR_KIND_ASSIGN,
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
        expr  base;
        expr *l;
        expr *r;
} expr_binary;

typedef struct {
        expr  base;
        char  op;
        expr *e;
} expr_unary;

typedef struct {
        expr        base;
        const char *id;
        expr       *e;
} expr_assign;

#endif
