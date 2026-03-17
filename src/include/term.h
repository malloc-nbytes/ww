#ifndef TERM_H_INCLUDED
#define TERM_H_INCLUDED

#include <termios.h>
#include <signal.h>

// Different control keys
#define CTRL_A 1
#define CTRL_B 2
#define CTRL_C 3
#define CTRL_D 4
#define CTRL_E 5
#define CTRL_F 6
#define CTRL_G 7
#define CTRL_H 8
#define CTRL_I 9
#define CTRL_J 10
#define CTRL_K 11
#define CTRL_L 12
#define CTRL_M 13
#define CTRL_N 14
#define CTRL_O 15
#define CTRL_P 16
#define CTRL_Q 17
#define CTRL_R 18
#define CTRL_S 19
#define CTRL_T 20
#define CTRL_U 21
#define CTRL_V 22
#define CTRL_W 23
#define CTRL_X 24
#define CTRL_Y 25
#define CTRL_Z 26

// Arrows
#define UP_ARROW    'A'
#define DOWN_ARROW  'B'
#define RIGHT_ARROW 'C'
#define LEFT_ARROW  'D'

/**
 * Parameter: ch -> the character to compare
 * Returns: whether `ch` is a newline
 * Description: Check if `ch` is a newline.
 */
#define ENTER(ch)     ((ch) == '\n')

/**
 * Parameter: ch -> the character to compare
 * Returns: whether `ch` is a backspace
 * Description: Check if `ch` is a backspace.
 */
#define BACKSPACE(ch) ((ch) == 8 || (ch) == 127)

/**
 * Parameter: ch -> the character to compare
 * Returns: whether `ch` is a tab
 * Description: Check if `ch` is a tab.
 */
#define TAB(ch)       ((ch) == '\t')

/**
 * Parameter: ch -> the character to compare
 * Returns: whether `ch` is an escape sequence
 * Description: Check if `ch` is an escape sequence.
 */
#define ESCSEQ(ch)    ((ch) == 27)

/**
 * Parameter: ch -> the character to compare
 * Returns: whether `ch` is a control sequence
 * Description: Check if `ch` is a control sequence.
 */
#define CSI(ch)       ((ch) == '[')

#define CURSOR_LEFT(n)  do { printf("\033[%dD", n); } while (0)
#define CURSOR_RIGHT(n) do { printf("\033[%dC", n); } while (0)
#define CURSOR_UP(n)    do { printf("\033[%dA", n); } while (0)
#define CURSOR_DOWN(n)  do { printf("\033[%dB", n); } while (0)

// Different input types.
typedef enum {
    INPUT_TYPE_CTRL = 0,
    INPUT_TYPE_ALT,
    INPUT_TYPE_ARROW,
    INPUT_TYPE_SHIFT_ARROW,
    INPUT_TYPE_NORMAL,
    INPUT_TYPE_UNKNOWN,
} input_type;

int        get_terminal_xy(size_t *win_width, size_t *win_height);
int        set_sigaction(struct sigaction *sa, void (*sa_handler_fun)(int), int signum);
int        enable_raw_terminal(int fd, struct termios *old_termios);
int        disable_raw_terminal(int fd, struct termios *old_termios);
input_type get_input(char *c);
void       clear_terminal(void);
void       gotoxy(int x, int y);
// dx, dy -> return coordinates after clearing line
void       clear_line(size_t dx, size_t dy);
void       anykey(void);
void       term_fullscrn(void);
void       term_exit_fullscrn(void);

#endif // TERM_H_INCLUDED
