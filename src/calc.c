#include "calc.h"
#include "lexer.h"
#include "parser.h"
#include "map.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

MAP_TYPE(const char *, calc_value *, calc_symtbl)

static calc_symtbl symtbl;

static calc_value *interpret_expr(expr *e);

static calc_value_int *
calc_value_int_alloc(int i)
{
        calc_value_int *v;

        v            = (calc_value_int *)malloc(sizeof(calc_value_int));
        v->i         = i;
        v->base.kind = CALC_VALUE_KIND_INT;

        return v;
}

static calc_value *
interpret_expr_binary(expr_binary *e)
{
        calc_value *res;
        calc_value *l;
        calc_value *r;

        res = l = r = NULL;

        if (!(l = interpret_expr(e->l)))
                return NULL;

        if (!(r = interpret_expr(e->r)))
                return NULL;

        calc_value_int *i0 = (calc_value_int *)l;
        calc_value_int *i1 = (calc_value_int *)r;

        if (!strcmp(e->op, "+")) {
                res = (calc_value *)calc_value_int_alloc(i0->i + i1->i);
        } else if (!strcmp(e->op, "-")) {
                res = (calc_value *)calc_value_int_alloc(i0->i - i1->i);
        } else if (!strcmp(e->op, "*")) {
                res = (calc_value *)calc_value_int_alloc(i0->i * i1->i);
        } else if (!strcmp(e->op, "/")) {
                res = (calc_value *)calc_value_int_alloc(i0->i / i1->i);
        } else {
                goto done;
        }

done:
        if (l)
                free(l);
        if (r)
                free(r);

        return res;
}

static inline calc_value *
interpret_expr_intlit(expr_intlit *e)
{
        return (calc_value *)calc_value_int_alloc(e->i);
}

static calc_value *
interpret_expr(expr *e)
{
        if (e->kind == EXPR_KIND_BINARY)
                return interpret_expr_binary((expr_binary *)e);
        if (e->kind == EXPR_KIND_INTLIT)
                return interpret_expr_intlit((expr_intlit *)e);
        return NULL;
}

static str
interpret(expr *e)
{
        calc_value *res;

        if (!(res = interpret_expr(e)))
                return str_from("invalid expression");

        if (res->kind == CALC_VALUE_KIND_INT) {
                char buf[256];
                sprintf(buf, "%d", ((calc_value_int *)res)->i);
                return str_from(buf);
        }

        return str_from("expression of unknown type");
}

str
calc(str math)
{
        lexer  l;
        expr  *e;
        str    res;

        l = lex_file((lexer_cfg){
                .fp   = "ww-calc",
                .src  = str_cstr(&math),
                .kwds = NULL,
                .mlop = NULL,
                .mlcl = NULL,
                .sl   = NULL,
                .bits = LEXERCFG_COPS,
        });

        // TODO: error checking the lexer.

        if (!(e = parse_expr(&l)))
                return str_from("invalid expression");

        res = interpret(e);

        lexer_destroy(&l);

        // TODO: free parser

        return res;
}

unsigned
calc_symtbl_hash(const char **s)
{
        return **s;
}

int
calc_symtbl_cmp(const char **s0,
                const char **s1)
{
        return strcmp(*s0, *s1);
}


void
calc_init(void)
{
        symtbl = calc_symtbl_create(calc_symtbl_hash, calc_symtbl_cmp);
}
