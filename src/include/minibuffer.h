#ifndef MINIBUFFER_H_INCLUDED
#define MINIBUFFER_H_INCLUDED

#include "str.h"
#include "ww.h"
#include "array.h"

char *minibuffer_input(const char *prompt,
                       const str  *autofill,
                       str_ar     *ac_items);
char *
minibuffer_completion_run(ww         *ed,
                          const char *label,
                          cstr_ar     items);

#endif // MINIBUFFER_H_INCLUDED
