#ifndef TUT_H_INCLUDED
#define TUT_H_INCLUDED

#include "buffer.h"
#include "ww.h"

extern char *g_tut01;

buffer *tut_alloc(ww *ed, const char *chapter);

#endif // TUT_H_INCLUDED
