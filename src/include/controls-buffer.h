#ifndef CONTROLS_BUFFER_H_INCLUDED
#define CONTROLS_BUFFER_H_INCLUDED

#include "colors.h"

#define HELP_DEF "*** " "Help Buffer ***\n" \
"\n" \
"ww is a text editor similar to vim or nano but with\n" \
"a *very* heavy Emacs influence. It's controls are mostly\n" \
"default Emacs keybindings (see controls).\n" \
"\n" \
"To begin, try finding a file with `C-x C-f' and choosing a file.\n" \
"\nTIP: Whenever there is a suggestion box, use `C-n' and `C-p' to\n" \
"     navigate through it.\n" \
"\n" \
"You can also invoke `M-x find-file' or if you want to browse the various commands.\n" \
"Feel free to navigate around this buffer for pactice (you cannot edit in here though).\n\n"

#define CONTROLS_DEF "*** Controls ***\n" \
"\n" \
"Note:\n" \
"\n" \
"- C = Control\n" \
"- M = Meta (alt)\n" \
"- Any capital letter on its own means <Shift><letter>.\n" \
"- any line with `*' means essential for tl;dr readers.\n" \
"\n" \
"Window Manipulation:\n" \
"\n" \
"M-x     = issue window command *\n" \
"C-x C-f = find file *\n" \
"C-x C-q = quit ww *\n" \
"\n" \
"Navigation:\n" \
"\n" \
"LEFT  | C-b = move cursor left *\n" \
"RIGHT | C-f = move cursor right *\n" \
"DOWN  | C-n = move cursor down *\n" \
"UP    | C-p = move cursor up *\n" \
"C-s         = search mode/next search instance *\n" \
"C-r         = search mode/previous search instance *\n" \
"M-f         = jump forward word\n" \
"M-b         = jump backward word\n" \
"M-{         = jump up one paragraph\n" \
"M-}         = jump down one paragraph\n" \
"M-<         = jump to top of buffer\n" \
"M->         = jump to bottom of buffer\n" \
"C-a         = go to begining of line\n" \
"C-e         = go to end of line\n" \
"C-v         = page down\n" \
"M-v         = page up\n" \
"M-m         = go to first non-space char on line\n" \
"\n" \
"Text Manipulation:\n" \
"\n" \
"BACKSPC | C-h = backspace one char *\n" \
"M-BACKSPC     = backspace word *\n" \
"ENTER   | C-j = insert newline *\n" \
"C-o           = insert newline (at cursor)\n" \
"C-d           = delete char under cursor *\n" \
"M-d           = delete word\n" \
"C-SPACE | C-c = begin/end selection *\n" \
"M-w           = copy selection *\n" \
"C-w           = cut selection\n" \
"C-y           = paste *\n" \
"C-k           = cut until end of line\n" \
"M-j           = combine lines\n" \
"M-g           = jump to line number\n" \
"\n" \
"Buffer Manipulation:\n" \
"\n" \
"C-x s = save buffer (must have write permissions) *\n" \
"C-x x = invoke compilation buffer\n" \
"C-x b = switch to different buffer *\n" \
"C-x k = kill current buffer\n" \
"\n" \
"Misc.:\n" \
"\n" \
"C-l = center view\n"

#endif
