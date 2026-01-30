#include "argument.h"
#include "flags.h"

#include <stdio.h>

int
main(int argc, char *argv[])
{
        if (argc <= 1)
                usage();

        char *filename = parse_args(argc, argv);

        return 0;
}
