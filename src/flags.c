#include "flags.h"
#include "colors.h"

#include <stdio.h>
#include <stdlib.h>

void
usage(void)
{
        printf("Usage: ww " YELLOW "[OPTIONS...]" RESET " " GREEN "[FILEPATH]" RESET " [^\\+\\d+$]\n");
        printf("Options:\n");
        printf("  " YELLOW "-h, --help" RESET "   display this message\n");
        exit(0);
}
