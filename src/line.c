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
line_from_nothing(void)
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
