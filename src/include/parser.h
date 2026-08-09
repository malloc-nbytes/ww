#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include "lex.h"
#include "grammar.h"

typedef struct {
        lexer *l;
        stmtp_ar stmts;
} parser;

parser parse_ast(lexer *l);

#endif // PARSER_H_INCLUDED