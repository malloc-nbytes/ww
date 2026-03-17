#include "calc.h"
#include "lexer.h"
#include "parser.h"
#include "map.h"

#include <assert.h>
#include <string.h>

MAP_TYPE(const char *, calc_value *, calc_symtbl)

static calc_symtbl symtbl;

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

        e = parse_expr(&l);

        lexer_destroy(&l);

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
