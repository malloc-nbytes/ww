#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include "str.h"

typedef enum {
        TK_EOF = 0,
        TK_ID,
        TK_KW,
        TK_LPAR,
        TK_RPAR,
        TK_INTL,
        TK_STRL,
        TK_SEMI,
} token_kind;

typedef struct {
        const char *fp;
        size_t      r;
        size_t      c;
} tloc;

typedef struct {
        tloc       loc;
        str        lx;
        token_kind k;
} token;

typedef struct {
        token *root;
        token *hd;
        token *tl;
        char *err;
} lexer;

lexer lex_file(const char *fp);
void  lexer_free(lexer *l);

#endif
