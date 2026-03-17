#include "parser.h"
#include "lexer.h"
#include "str.h"

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
expr_binary_alloc(expr       *l,
                  const char *op,
                  expr       *r)
{
        expr_binary *e;

        e            = (expr_binary *)malloc(sizeof(expr_binary));
        e->l         = l;
        e->op        = strdup(op);
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

static expr *
parse_primary_expr(lexer *l)
{
        expr *left;

        left = NULL;

        while (1) {
                const token *hd = lexer_hd(l);

                switch (hd->k) {
                case TK_ID: {
                        const token *i = lexer_next(l);
                        left = (expr *)expr_ident_alloc(str_cstr(&i->lx));
                } break;
                case TK_INTL: {
                        const token *i = lexer_next(l);
                        left = (expr *)expr_intlit_alloc(atoi(str_cstr(&i->lx)));
                } break;
                default: break;
                }
        }

        return left;
}

static expr *
parse_unary_expr(lexer *l)
{
        token *cur;

        cur = lexer_hd(l);

        while (cur && (cur->k == TK_MINUS
                        || cur->k == TK_PLUS
                        || cur->k == TK_BANG)) {
                token *op = lexer_next(l);
                expr *rhs = (expr *)parse_expr(l);
                return (expr *)expr_unary_alloc(str_cstr(&op->lx)[0], rhs);
        }

        return parse_primary_expr(l);
}

static expr *
parse_multiplicative_expr(lexer *l)
{
        expr        *lhs;
        const token *cur;

        lhs = parse_unary_expr(l);
        cur = lexer_hd(l);

        while (cur && (cur->k == TK_AST
                        || cur->k == TK_FORSL
                        || cur->k == TK_PERC)) {
                token *op        = lexer_next(l);
                expr  *rhs       = parse_unary_expr(l);
                expr_binary *bin = expr_binary_alloc(lhs, str_cstr(&op->lx), rhs);
                lhs              = (expr *)bin;
                cur              = lexer_hd(l);
        }

        return lhs;
}

static expr *
parse_additive_expr(lexer *l)
{
        expr        *lhs;
        const token *cur;

        lhs = parse_multiplicative_expr(l);
        cur = lexer_hd(l);

        while (cur && (cur->k == TK_PLUS
                        || cur->k == TK_MINUS)) {
                token *op        = lexer_next(l);
                expr *rhs        = parse_multiplicative_expr(l);
                expr_binary *bin = expr_binary_alloc(lhs, str_cstr(&op->lx), rhs);
                lhs              = (expr *)bin;
                cur              = lexer_hd(l);
        }

        return lhs;
}

static expr *
parse_equalitative_expr(lexer *l)
{
        expr        *lhs;
        const token *cur;

        lhs = parse_additive_expr(l);
        cur = lexer_hd(l);

        while (cur && (cur->k == TK_EQEQ
                        || cur->k == TK_GTE
                        || cur->k == TK_GT
                        || cur->k == TK_LTE
                        || cur->k == TK_LT
                        || cur->k == TK_BANGEQ)) {
                token *op        = lexer_next(l);
                expr *rhs        = parse_additive_expr(l);
                expr_binary *bin = expr_binary_alloc(lhs, strdup(str_cstr(&op->lx)), rhs);
                lhs              = (expr *)bin;
                cur              = lexer_hd(l);
        }

        return lhs;
}

static expr *
parse_logical_expr(lexer *l)
{
        expr        *lhs;
        const token *cur;

        lhs = parse_equalitative_expr(l);
        cur = lexer_hd(l);

        while (cur && (cur->k == TK_AMPAMP
                        || cur->k == TK_PIPEPIPE)) {
                token *op        = lexer_next(l);
                expr *rhs        = parse_equalitative_expr(l);
                expr_binary *bin = expr_binary_alloc(lhs, strdup(str_cstr(&op->lx)), rhs);
                lhs              = (expr *)bin;
                cur              = lexer_hd(l);
        }

        return lhs;
}

static expr *
parse_assignment_expr(lexer *l)
{
        expr        *lhs;
        const token *cur;

        lhs = parse_logical_expr(l);

        if (!(cur = lexer_hd(l)))
                return lhs;

        switch (cur->k) {
        case TK_PLUSEQ:
        case TK_MINUSEQ:
        case TK_ASTEQ:
        case TK_FORSLEQ:
        case TK_PERCEQ:
        case TK_AMPEQ:
        case TK_PIPEEQ:
        case TK_UPTICKEQ:
        case TK_EQ: {
                const token *op  = lexer_next(l);
                expr        *rhs = parse_assignment_expr(l);
                return (expr *)expr_binary_alloc(lhs, str_cstr(&op->lx), rhs);
        } break;
        default: break;
        }

        return lhs;
}

expr *
parse_expr(lexer *l)
{
        return parse_assignment_expr(l);
}
