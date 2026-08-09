#include "location.h"

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
