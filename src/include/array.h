#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <stdlib.h>

#define DYN_ARRAY_TYPE(ty, name) \
    typedef struct name {        \
        ty *data;                \
        size_t len, cap;         \
    } name

#define dyn_array_empty(arr_ty)                 \
        (arr_ty) {                              \
                .data = NULL,                   \
                .len = 0,                       \
                .cap = 0,                       \
        }

#define dyn_array_init_type(da)                 \
    do {                                        \
        (da).data = malloc(sizeof(*(da).data)); \
        (da).cap = 1;                           \
        (da).len = 0;                           \
    } while (0)

#define dyn_array(ty, name)                                        \
    struct {                                                       \
        ty *data;                                                  \
        size_t len, cap;                                           \
    } (name) = { .data = (typeof(ty) *)malloc(sizeof(ty)), .len = 0, .cap = 1 };

#define dyn_array_append(da, value)                                     \
    do {                                                                \
        if ((da).len >= (da).cap) {                                     \
            (da).cap = (da).cap ? (da).cap * 2 : 2;                     \
            (da).data = (typeof(*((da).data)) *)                        \
                realloc((da).data,                                      \
                        (da).cap * sizeof(*((da).data)));               \
        }                                                               \
        (da).data[(da).len++] = (value);                                \
    } while (0)

#define dyn_array_free(da)       \
    do {                         \
        if ((da).data != NULL) { \
                free((da).data); \
        }                        \
        (da).len = (da).cap = 0; \
    } while (0)

#define dyn_array_at_s(da, i)                                      \
    ((i) < (da).len ? (da).data[i] : (fprintf(stderr,              \
    "[dyn_array error]: index %zu is out of bounds (len = %zu)\n", \
    (size_t)(i), (size_t)(da).len), exit(1), (da).data[0]))

#define dyn_array_at(da, i) ((da).data[i])

#define dyn_array_clear(da) (da).len = 0;

#define dyn_array_rm_at(da, idx) \
    do {                                                     \
        for (size_t __i_ = (idx); __i_ < (da).len-1; ++__i_) \
            (da).data[__i_] = (da).data[__i_+1];             \
        (da).len--;                                          \
    } while (0)

#define dyn_array_insert_at(da, idx, value) \
    do { \
        if ((idx) > (da).len) { \
            fprintf(stderr, \
                "[dyn_array error]: insert index %zu is out of bounds (len = %zu)\n", \
                (size_t)(idx), (size_t)(da).len); \
            exit(1); \
        } \
        if ((da).len >= (da).cap) { \
            (da).cap = (da).cap ? (da).cap * 2 : 2; \
            (da).data = (typeof(*((da).data)) *) \
                realloc((da).data, (da).cap * sizeof(*((da).data))); \
            if ((da).data == NULL) { \
                fprintf(stderr, "[dyn_array error]: realloc failed\n"); \
                exit(1); \
            } \
        } \
        for (size_t __i = (da).len; __i > (idx); --__i) { \
            (da).data[__i] = (da).data[__i - 1]; \
        } \
        (da).data[idx] = (value); \
        (da).len++; \
    } while (0)

// Common types for arrays.

DYN_ARRAY_TYPE(int,      int_array);
DYN_ARRAY_TYPE(char,     char_array);
DYN_ARRAY_TYPE(char *,   cstr_array);
DYN_ARRAY_TYPE(size_t,   size_t_array);
DYN_ARRAY_TYPE(float,    float_array);
DYN_ARRAY_TYPE(double,   double_array);
DYN_ARRAY_TYPE(long,     long_array);
DYN_ARRAY_TYPE(unsigned, unsigned_array);
DYN_ARRAY_TYPE(void *,   void_ptr_array);
DYN_ARRAY_TYPE(const char *, const_cstr_array);

#endif // ARRAY_H_INCLUDED
