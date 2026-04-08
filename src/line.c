#include "line.h"
#include "str.h"

line
line_create(void)
{
        return (line) {
                .txt = str_from("\n"),
        };
}

line
line_create_nothing(void)
{
        return (line) {
                .txt = str_create(),
        };
}

line
line_from(str s)
{
        return (line) {
                .txt = s,
        };
}

line
line_from_cstr(const char *s)
{
        return (line) {
                .txt = str_from(s),
        };
}

void
line_append(line *ln, char ch)
{
        str_append(&ln->txt, ch);
}

line_ar
lines_from(char *chars)
{
        line_ar ar;
        str     buf;

        ar  = array_empty(line_ar);
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
