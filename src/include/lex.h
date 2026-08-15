#ifndef LEX_H_INCLUDED
#define LEX_H_INCLUDED

#include "line.h"
#include "array.h"
#include "map.h"
#include "location.h"
#include "sv.h"

#include <stddef.h>

typedef enum {
        TOKEN_KIND_EOF = 0,
        TOKEN_KIND_LCURLY,
        TOKEN_KIND_RCURLY,
        TOKEN_KIND_LSQR,
        TOKEN_KIND_RSQR,
        TOKEN_KIND_COLON,
        TOKEN_KIND_COMMA,
        TOKEN_KIND_STRING_LITERAL,
        TOKEN_KIND_NUMBER_LITERAL,
        TOKEN_KIND_IDENTIFIER,
        TOKEN_KIND_KEYWORD,
} token_kind;

MAP_DEFINE(const char *, token_kind, opmap);

typedef struct token {
        token_kind k;
        sv lx;
        location loc;
        struct token *n;
} token;

ARRAY_DEFINE(token *, tokenp_ar);

typedef enum {
        LEXER_CFG_MODE_JSON = 0,
} lexer_cfg_mode;

typedef struct {
        lexer_cfg_mode mode;
        opmap ops;
        linep_ar lns;
        const char *path;
} lexer_cfg;

typedef struct {
        lexer_cfg cfg;
        tokenp_ar tokens;
        size_t cursor;
} lexer;

lexer lex_file(lexer_cfg cfg);
void lexer_dump(const lexer *l);
token *lexer_peek(const lexer *l);
token *lexer_next(lexer *l);
void lexer_discard(lexer *l);

#endif // LEX_H_INCLUDED
