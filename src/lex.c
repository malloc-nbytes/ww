#include "lex.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>

lexer
lex_file(lexer_cfg cfg)
{
        lexer l;
        size_t r, c, n;

        l = (lexer) {
                .cfg = cfg,
                .tokens = array_empty(tokenp_ar),
                .cursor = 0,
        };

        r = 1, c = 0,
        n = strlen(l.cfg.path);

        for (size_t i = 0; i < l.cfg.lns.len; ++i) {
                const line *ln    = l.cfg.lns.data[i];
                const str  *s     = &ln->txt;
                const char *chars = ln->txt.chars;

                for (size_t j = 0; j < s->len; ++j) {
                        char ch = s->chars[j];
                        if (ch == '\n') {
                                c = 1;
                                ++r;
                                ++i;
                        } else if (isspace(ch)) {
                                ++c;
                                ++i;
                        } else if (isalpha(ch) || ch == '_') {
                                assert(0 && "identifiers unimplemented");
                        } else if (ch == '"') {
                                assert(0 && "quotes unimplemented");
                        } else if (ch == '\'') {
                                assert(0 && "single quotes unimplemented");
                        } else if (isdigit(ch)) {
                                assert(0 && "numbers unimplemented");
                        } else {
                                assert(0 && "operators");
                        }
                }
        }

        return l;
}
