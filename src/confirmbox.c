#include "confirmbox.h"
#include "term.h"
#include "colors.h"

#include <stdio.h>

int
confirmbox(const char *prompt,
           const char *extra_option)
{
        (void)prompt;
        (void)extra_option;

        clear_terminal();

        int option = 0;

        hide_cursor();

        while (1) {
                gotoxy(0, 0);

                printf("%s\n  ", prompt);

                if (option == 1) printf(YELLOW BOLD "[");
                printf("Yes");
                if (option == 1) printf("]" RESET);

                printf("      ");

                if (option == 0) printf(YELLOW BOLD "[");
                printf("No");
                if (option == 0) printf("]" RESET);

                fflush(stdout);

                char ch;
                input_type ty;

                switch (ty = get_input(&ch)) {
                case INPUT_TYPE_NORMAL: {
                        if (ch == '\n')
                                goto done;
                } break;
                case INPUT_TYPE_ARROW: {
                        if (ch == RIGHT_ARROW && option > 0)
                                --option;
                        if (ch == LEFT_ARROW && option < 1)
                                ++option;
                } break;
                default: break;
                }
        }

done:
        show_cursor();
        return option;
}

