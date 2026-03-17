#ifndef CALC_H_INCLUDED
#define CALC_H_INCLUDED

#include "str.h"

typedef enum {
        CALC_VALUE_KIND_INT = 0,
} calc_value_kind;

typedef struct {
        calc_value_kind kind;
} calc_value;

typedef struct {
        calc_value base;
        int i;
} calc_value_int;

void calc_init(void);
str  calc(str math);

#endif
