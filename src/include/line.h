#ifndef LINE_H_INCLUDED
#define LINE_H_INCLUDED

#include "str.h"
#include "array.h"

typedef struct {
        str txt;
} line;

ARRAY_DEFINE(line *, linep_ar);

line    *line_alloc(void);
line*    line_from(str s);
line*    line_create_nothing(void);
line*    line_from_cstr(const char *s);
void    line_append(line *ln, char ch);
linep_ar lines_from(char *chars);
void    line_free(line *ln);

#endif // LINE_H_INCLUDED
