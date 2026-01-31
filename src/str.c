#include "str.h"

#include <stdlib.h>
#include <string.h>

str
str_create(void)
{
        return (str) {
                .chars = NULL,
                .len   = 0,
                .cap   = 0,
        };
}

str
str_from(const char *chars)
{
        str s = str_create();
        str_concat(&s, chars);
        return s;
}

inline const char *
str_cstr(const str *s)
{
        return s->chars;
}

void
str_append(str *s, char c)
{
        if (s->len >= s->cap) {
                s->cap = s->cap ? s->cap*2 : 2;
                s->chars = (char *)realloc(s->chars, s->cap);
                memset(s->chars + s->len+1, 0, s->cap - s->len);
        }
        s->chars[s->len++] = c;
}

void
str_concat(str *s, const char *chars)
{
        for (size_t i = 0; chars[i]; ++i)
                str_append(s, chars[i]);
}

void
str_clear(str *s)
{
        memset(s->chars, 0, s->cap);
        s->len = 0;
}

void
str_overwrite(str *s, const char *repl)
{
        str_clear(s);
        str_concat(s, repl);
}

inline size_t
str_len(const str *s)
{
        return s->len;
}

void
str_destroy(str *s)
{
        free(s->chars);
        s->chars = NULL;
        s->len   = 0;
        s->cap   = 0;
}
