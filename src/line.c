#include "line.h"
#include "str.h"
#include "mem.h"

line *
line_alloc(void)
{
        line *l;

        l = (line *)alloc(sizeof(line));

        l->txt = str_from("\n");

        return l;
}

line *
line_create_nothing(void)
{
        line *l;

        l      = (line *)alloc(sizeof(line));
        l->txt = str_create();

        return l;
}

line *
line_from(str s)
{
        line *l;

        l      = (line *)alloc(sizeof(line));
        l->txt = s;

        return l;
}

line *
line_from_cstr(const char *s)
{
        line *l;

        l      = (line *)alloc(sizeof(line));
        l->txt = str_from(s);

        return l;
}

void
line_append(line *ln, char ch)
{
        str_append(&ln->txt, ch);
}

linep_ar
lines_from(char *chars)
{
        linep_ar ar;
        str      buf;

        ar  = array_empty(linep_ar);
        buf = str_create();

        for (size_t i = 0; chars[i]; ++i) {
                str_append(&buf, chars[i]);
                if (chars[i] == '\n') {
                        array_append(ar, line_from(str_dup(buf)));
                        str_clear(&buf);
                }
        }

        if (buf.len > 0) {
                str_append(&buf, '\n');
                array_append(ar, line_from(str_dup(buf)));
        }

        str_destroy(&buf);

        return ar;
}

void
line_destroy(line *ln)
{
        str_destroy(&ln->txt);
        free(ln);
}
