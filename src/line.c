#include "line.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

line *
line_alloc(int no)
{
        line *l = (line *)malloc(sizeof(line));
        l->s    = str_create();
        l->no   = no;
        return l;
}

line *
line_from(int no, str s)
{
        line *l = (line *)malloc(sizeof(line));
        l->s    = s;
        l->no   = no;
        return l;
}

line *
line_from_cstr(int no, const char *s)
{
        line *l = (line *)malloc(sizeof(line));
        l->s    = str_from(s);
        l->no   = no;
        return l;
}

line_array
lines_of_cstr(const char *s)
{
        line_array lns;
        char_array buf;

        lns = dyn_array_empty(line_array);
        buf = dyn_array_empty(char_array);

        for (size_t i = 0, no = 1; s[i]; ++i) {
                dyn_array_append(buf, s[i]);
                if (s[i] == 10) {
                        dyn_array_append(buf, 0);
                        dyn_array_append(lns, line_from_cstr(no++, buf.data));
                        dyn_array_clear(buf);
                }
        }

        dyn_array_free(buf);

        return lns;
}

void
line_free(line *ln)
{
        str_destroy(&ln->s);
        free(ln);
        ln = NULL;
}
