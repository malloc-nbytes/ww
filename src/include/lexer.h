#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include "str.h"

#include <stdint.h>

#define LEXERCFG_NOBITS              (0)
#define LEXERCFG_TRACK_NEWLINES      (1 << 0)
#define LEXERCFG_TRACK_TABS          (1 << 1)
#define LEXERCFG_TRACK_SPACES        (1 << 2)
#define LEXERCFG_TRACK_CHARS_AS_STRS (1 << 3)
#define LEXERCFG_COPS                (1 << 4)

#define LEXER_C_KWDS { \
        "if", \
        "else", \
        "while", \
        "for", \
        "int", \
        "char", \
        "float", \
        "double", \
        "auto", \
        "bool", \
        "break", \
        "case", \
        "default", \
        "const", \
        "constexpr", \
        "continue", \
        "do", \
        "enum", \
        "extern", \
        "true", \
        "false", \
        "goto", \
        "inline", \
        "long", \
        "nullptr", \
        "NULL", \
        "register", \
        "restrict", \
        "return", \
        "short", \
        "unsigned", \
        "signed", \
        "sizeof", \
        "static", \
        "static_assert", \
        "struct", \
        "switch", \
        "thread_local", \
        "typedef", \
        "typeof", \
        "typeof_unequal", \
        "union", \
        "void", \
        "volatile", \
        "_Alignas", \
        "_Alignof", \
        "_Atomic", \
        "_BitInt", \
        "_Bool", \
        "_Complex", \
        "_Decimal128", \
        "_Decimal32", \
        "_Decimal16", \
        "_Generic", \
        "_Imaginary", \
        "_Noreturn", \
        "_Static_assert", \
        "_Thread_local", \
}

#define LEXER_PY_KWDS { \
                "False", \
                "None", \
                "True", \
                "and", \
                "as", \
                "assert", \
                "async", \
                "await", \
                "break", \
                "class", \
                "continue", \
                "def", \
                "del", \
                "elif", \
                "else", \
                "except", \
                "finally", \
                "for", \
                "from", \
                "global",\
                "if", \
                "import", \
                "in", \
                "is", \
                "lambda", \
                "nonlocal", \
                "not", \
                "or", \
                "pass", \
                "raise", \
                "return", \
                "try", \
                "while", \
                "with", \
                "yield", \
        }

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
        char  *err;
} lexer;

typedef struct {
        const char  *fp;
        const char  *src;
        const char **kwds;
        const char  *multi_comm_op;
        const char *multi_comm_cl;
        const char *single_comm;
        uint32_t     bits;
} lexer_cfg;

lexer_cfg lexer_cfg_from_filext(const char *ext);
lexer     lex_file(lexer_cfg cfg);
void      lexer_free(lexer *l);

#endif
