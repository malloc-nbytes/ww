#ifndef LEX_H_INCLUDED
#define LEX_H_INCLUDED

#include "array.h"

typedef struct {
} token_kind;

typedef struct {
} token;

ARRAY_DEFINE(token *, tokenp_ar);

typedef struct {
} lexer_cfg;

typedef struct {
} lexer;

lexer lex(const char *path, const char *src, lexer_cfg cfg);

#endif // LEX_H_INCLUDED
