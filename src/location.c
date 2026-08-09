#include "location.h"

#include <string.h>
#include <stdio.h>

location
location_from(size_t       r,
              size_t       c,
              const char  *path)
{
        return (location) {
                .r = r,
                .c = c,
                .path = path,
        };
}

const char *
loc_fmt_cstr(location loc)
{
        static char buf[256] = {0};
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s:%zu:%zu:", loc.path, loc.r, loc.c);
        return buf;
}