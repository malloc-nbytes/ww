#ifndef TUT_H_INCLUDED
#define TUT_H_INCLUDED

#include "buffer.h"
#include "ww.h"

#define TUT_CH1_NAME "ww-tut-ch1 - Basic Navigation"
#define TUT_CH2_NAME "ww-tut-ch2 - Basic Editing"

extern char *g_tut01;

buffer *tut_alloc(ww *ed, const char *chapter);

#endif // TUT_H_INCLUDED
