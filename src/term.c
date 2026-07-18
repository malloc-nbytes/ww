/*
 * ww: a simple editor
 * Copyright (C) 2026 malloc-nbytes
 * Contact: zdhdev@yahoo.com

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "term.h"

static char
get_char(void)
{
        char ch;
        ssize_t _ = read(STDIN_FILENO, &ch, 1);
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

                                // CSI 200~ -> bracketed paste begins
                                // CSI 201~ -> bracketed paste ends
                                if (next1 >= '0' && next1 <= '9') {
                                        int value = next1 - '0';

                                        while (1) {
                                                int ch = get_char();

                                                if (ch >= '0' && ch <= '9') {
                                                        value = value * 10 + (ch - '0');
                                                }
                                                else if (ch == '~') {
                                                        switch (value) {
                                                                case 200:
                                                                        return INPUT_TYPE_PASTE_BEGIN;
                                                                case 201:
                                                                        return INPUT_TYPE_PASTE_END;
                                                                default:
                                                                        return INPUT_TYPE_UNKNOWN;
                                                        }
                                                }
                                                else if (ch == ';') {
                                                        /* Modifier sequence (e.g. Shift+Arrow). */
                                                        int modifier = get_char();
                                                        int arrow = get_char();

                                                        if (modifier == '2') {
                                                                switch (arrow) {
                                                                        case UP_ARROW:
                                                                                *c = UP_ARROW;
                                                                                return INPUT_TYPE_SHIFT_ARROW;
                                                                        case DOWN_ARROW:
                                                                                *c = DOWN_ARROW;
                                                                                return INPUT_TYPE_SHIFT_ARROW;
                                                                        case RIGHT_ARROW:
                                                                                *c = RIGHT_ARROW;
                                                                                return INPUT_TYPE_SHIFT_ARROW;
                                                                        case LEFT_ARROW:
                                                                                *c = LEFT_ARROW;
                                                                                return INPUT_TYPE_SHIFT_ARROW;
                                                                        default:
                                                                                return INPUT_TYPE_UNKNOWN;
                                                                }
                                                        }

                                                        return INPUT_TYPE_UNKNOWN;
                                                }
                                                else {
                                                        return INPUT_TYPE_UNKNOWN;
                                                }
                                        }
                                }
                                else {
                                        /* Regular arrow keys */
                                        switch (next1) {
                                                case UP_ARROW:
                                                case DOWN_ARROW:
                                                case LEFT_ARROW:
                                                case RIGHT_ARROW:
                                                        *c = (char)next1;
                                                        return INPUT_TYPE_ARROW;
                                                default:
                                                        return INPUT_TYPE_UNKNOWN;
                                        }
                                }
                        }
                        else {
                                /* Alt+key */
                                *c = (char)next0;
                                return INPUT_TYPE_ALT;
                        }
                }
                else if (*c >= CTRL_A && *c <= CTRL_Z && *c != CTRL_J) {
                        return INPUT_TYPE_CTRL;
                }
                else {
                        return INPUT_TYPE_NORMAL;
                }
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
        raw.c_lflag &= (tcflag_t)~(ECHO | ICANON);
        raw.c_iflag &= (tcflag_t)~IXON;

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
gotoxy(unsigned x, unsigned y)
{
        printf("\033[%d;%dH", y+1, x+1);
}

void
clear_line(size_t dx, size_t dy)
{
        printf("\033[2K");
        printf("\033[0G");
        gotoxy((unsigned)dx, (unsigned)dy);
        fflush(stdout);
}

void
anykey(void)
{
        printf("Press any key to continue...\n");
        char _;
        (void)get_input(&_);
}

void
term_fullscrn(void)
{
        printf("\x1b[?1049h");
}

void
term_exit_fullscrn(void)
{
        printf("\x1b[?1049l");
}

void
clear_line_imm(void)
{
        printf("\033[2K");
        printf("\033[0G");
        fflush(stdout);
}

void
enable_mousewheel_capture(void)
{
        printf("\033[?1000h");
}

void
disable_mousewheel_capture(void)
{
        printf("\033[?1000l");
}

void
hide_cursor(void)
{
        printf("\033[?25l");
}

void
show_cursor(void)
{
        printf("\033[?25h");
}

void
enable_bracketed_paste(void)
{
        printf("\033[?2004h");
        fflush(stdout);
}

void
disable_bracketed_paste(void)
{
        printf("\033[?2004l");
        fflush(stdout);
}

