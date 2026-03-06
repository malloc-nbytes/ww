#include "flags.h"
#include "colors.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

void
usage(void)
{
        printf("ww version 1.0, Copyright (C) 2025 malloc-nbytes.\n");
        printf("ww comes with ABSOLUTELY NO WARRANTY.\n");
        printf("This is free software, and you are welcome to redistribute it\n");
        printf("under certain conditions; see command `copying`.\n\n");

        printf("Compiler information:\n");
        printf("| cc: " COMPILER_NAME "\n");
        printf("| path: " COMPILER_PATH "\n");
        printf("| ver.: " COMPILER_VERSION "\n\n");

        printf("Usage: ww " YELLOW "[OPTIONS...]" RESET " " GREEN "[FILEPATH]" RESET " [^\\+\\d+$]\n");
        printf("Options:\n");
        printf("  " YELLOW "-h, --help" RESET "      display this message\n");
        printf("  " YELLOW "-v, --version" RESET "   show the version\n");
        exit(0);
}

void
version(void)
{
        printf("ww-v" VERSION "\n");
        exit(0);
}
