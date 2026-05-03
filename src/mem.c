#include "mem.h"
#include "error.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>

uint8_t *
alloc(size_t b)
{
        uint8_t *p;

        if (!(p = malloc(b)))
                fatal("could not alloc `%zu' bytes: %s", b, strerror(errno));

        return p;
}
