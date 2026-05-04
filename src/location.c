#include "location.h"

#include <string.h>
#include <stdlib.h>

location
location_from(const char *path,
              unsigned   r,
              unsigned   c)
{
        return (location) {
                .path = strdup(path),
                .r    =  r,
                .c    =  c,
        };
}

void
location_destroy(location *loc)
{
        free(loc->path);
}
