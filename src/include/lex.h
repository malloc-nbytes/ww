#ifndef LEX_H_INCLUDED
#define LEX_H_INCLUDED

#include "str.h"
#include "location.h"
#include "array.h"

#include <stdint.h>
#include <stddef.h>

#define LEXER_CFG_NO_BITS        (0)
#define LEXER_CFG_TRACK_NEWLINES (1 << 0)
#define LEXER_CFG_TRACK_TABS     (1 << 1)
#define LEXER_CFG_TRACK_SPACES   (1 << 2)
#define LEXER_CFG_CHARS_AS_STRS  (1 << 3)
#define LEXER_CFG_C_OPERATORS    (1 << 4)

typedef enum {
        TK_EOF = 0,
        TK_IDENT,
        TK_STRLIT,
        TK_INTLIT,
        TK_LPAREN,
        TK_RPAREN,
        TK_LSQR,
        TK_RSQR,
        TK_LCURLY,
        TK_RCURLY,
        TK_SEMICOLON,
        TK_PERIOD,
        TK_PLUS,
        TK_MINUS,
        TK_ASTERISK,
        TK_FORWARDSLASH,
        TK_PERCENT,
        TK_BANG,
        TK_HASH,
        TK_UPTICK,
        TK_AMPERSAND,
        TK_PIPE,
        TK_EQ,
        TK_LT,
        TK_GT,
        TK_GTEQ,
        TK_LTEQ,
        TK_BANGEQ,
        TK_UPTICKEQ,
        TK_PIPEEQ,
        TK_DOUBLEPIPE,
        TK_DOUBLEAMPERSAND,
        TK_AMPERSANDEQ,
        TK_EQEQ,
        TK_PLUSEQ,
        TK_MINUSEQ,
        TK_ASTERISKEQ,
        TK_FORWARDSLASHEQ,
        TK_BACKSLASH,
} token_kind;

typedef struct {
        token_kind kind;
        str        lx;
        location   loc;
} token;

ARRAY_DEFINE(token *, tokenp_ar);

typedef struct {
        const char *path;
        const char *src;
        struct {
                const char *single;
                const char *multi_begin;
                const char *multi_end;
        } comment;
        const char **kwds;
        uint32_t     bits;
} lexer_cfg;

typedef struct {
        tokenp_ar tokens;
        size_t    pos;
        lexer_cfg cfg;
} lexer;

lexer lex(lexer_cfg cfg);

#endif // LEX_H_INCLUDED
