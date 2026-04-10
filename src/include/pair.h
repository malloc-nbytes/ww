#ifndef PAIR_H_INCLUDED
#define PAIR_H_INCLUDED

#define PAIR_DEFINE(t0, t1, name) \
        typedef struct { \
                t0 l; \
                t1 r; \
        } name; \
        \
        name name##_create(t0 l, t1 r)

#define PAIR_IMPL(t0, t1, name) \
        name \
        name##_create(t0 l, t1 r) \
        { \
                return (name) { .l = l, .r = r, }; \
        }

#endif // PAIR_H_INCLUDED
