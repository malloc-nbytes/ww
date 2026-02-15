#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "term.h"

static char
get_char(void)
{
        char ch;
        int _ = read(STDIN_FILENO, &ch, 1);
        (void)_;
        return ch;
}

input_type
get_input(char *c)
{
        assert(c);
        while (1) {
                *c = get_char();
                if (ESCSEQ(*c)) {
                        int next0 = get_char();
                        if (CSI(next0)) {
                                int next1 = get_char();
                                if (next1 >= '0' && next1 <= '9') { // Modifier key detected
                                        int semicolon = get_char();
                                        if (semicolon == ';') {
                                                int modifier = get_char();
                                                int arrow_key = get_char();
                                                if (modifier == '2') { // Shift modifier
                                                        switch (arrow_key) {
                                                        case 'A': *c = UP_ARROW;    return INPUT_TYPE_SHIFT_ARROW;
                                                        case 'B': *c = DOWN_ARROW;  return INPUT_TYPE_SHIFT_ARROW;
                                                        case 'C': *c = RIGHT_ARROW; return INPUT_TYPE_SHIFT_ARROW;
                                                        case 'D': *c = LEFT_ARROW;  return INPUT_TYPE_SHIFT_ARROW;
                                                        default: return INPUT_TYPE_UNKNOWN;
                                                        }
                                                }
                                        }
                                        return INPUT_TYPE_UNKNOWN;
                                } else { // Regular arrow key
                                        switch (next1) {
                                        case DOWN_ARROW:
                                        case RIGHT_ARROW:
                                        case LEFT_ARROW:
                                        case UP_ARROW:
                                                *c = next1;
                                                return INPUT_TYPE_ARROW;
                                        default:
                                                return INPUT_TYPE_UNKNOWN;
                                        }
                                }
                        } else { // [ALT] key
                                *c = next0;
                                return INPUT_TYPE_ALT;
                        }
                }
                else if (*c >= CTRL_A && *c <= CTRL_Z && *c != CTRL_J) {
                        return INPUT_TYPE_CTRL;
                }
                else return INPUT_TYPE_NORMAL;
        }
        return INPUT_TYPE_UNKNOWN;
}


int
get_terminal_xy(size_t *win_width,
                size_t *win_height)
{
        if (!win_width && !win_height) return 0;

        if (win_width)  *win_width = 0;
        if (win_height) *win_height = 0;

        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
                if (win_width)  *win_width = w.ws_col;
                if (win_height) *win_height = w.ws_row-1;
        } else {
                return 0;
        }

        return 1;
}

int
set_sigaction(struct sigaction *sa,
              void             (*sa_handler_fun)(int),
              int              signum)
{
        sa->sa_handler = sa_handler_fun;
        sa->sa_flags = 0;
        sigemptyset(&sa->sa_mask);
        if (sigaction(signum, sa, NULL) == -1) {
                perror("sigaction");
                return 0;
        }
        return 1;
}

int
enable_raw_terminal(int               fd,
                    struct termios   *old_termios)
{
        struct termios raw;

        // Get current terminal attributes
        if (tcgetattr(fd, old_termios) == -1) {
                perror("tcgetattr failed");
                fprintf(stderr, "Could not get terminal attributes\n");
                return 0;
        }

        // Copy current attributes
        raw = *old_termios;

        // Modify for raw mode: disable ECHO, ICANON, and IXON
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_iflag &= ~IXON;

        // Apply new settings
        if (tcsetattr(fd, TCSANOW, &raw) == -1) {
                perror("tcsetattr failed");
                fprintf(stderr, "Could not set terminal to raw mode\n");
                return 0;
        }

        return 1;
}

int
disable_raw_terminal(int             fd,
                     struct termios *old_termios)
{
        struct termios current;

        // Get current terminal attributes
        if (tcgetattr(fd, &current) == -1) {
                perror("tcgetattr failed");
                fprintf(stderr, "Could not get terminal attributes\n");
                return 0;
        }

        // Check if terminal is already in non-raw mode (ECHO and ICANON are enabled)
        if (current.c_lflag & ECHO && current.c_lflag & ICANON) {
                // Terminal is already in non-raw mode, no need to restore
                return 1;
        }

        // Restore original terminal settings
        if (tcsetattr(fd, TCSANOW, old_termios) == -1) {
                perror("tcsetattr failed");
                fprintf(stderr, "Could not restore terminal settings\n");
                return 0;
        }

        return 1;
        //return tcsetattr(fd, TCSANOW, old_termios) != -1;
}

void
clear_terminal(void)
{
        printf("\033[2J");
        printf("\033[H");
        fflush(stdout);
}

void
gotoxy(int x, int y)
{
        printf("\033[%d;%dH", y+1, x+1);
}

void
clear_line(size_t dx, size_t dy)
{
        printf("\033[2K");
        printf("\033[0G");
        gotoxy(dx, dy);
        fflush(stdout);
}
