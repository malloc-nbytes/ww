#ifndef LOCATION_H_INCLUDED
#define LOCATION_H_INCLUDED

#include <stddef.h>

typedef struct {
        size_t r, c;
        const char *path;
} location;

location location_from(size_t r, size_t c, const char *path);
const char *loc_fmt_cstr(location loc);

#endif // LOCATION_H_INCLUDED
