#ifndef MINIBUFFER_H_INCLUDED
#define MINIBUFFER_H_INCLUDED

#include "str.h"

char *minibuffer_input(const char *prompt,
                       const str  *autofill,
                       str_ar     *ac_items);

#endif // MINIBUFFER_H_INCLUDED
