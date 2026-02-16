#ifndef PAIR_H_INCLUDED
#define PAIR_H_INCLUDED

#define PAIR_TYPE(ty1, ty2, name) \
        typedef struct { \
                ty1 l; \
                ty2 r; \
        } name; \
        \
        name name##_create(ty1 l, ty2 r); \

#define PAIR_IMPL(ty1, ty2, name) \
        name \
        name##_create(ty1 l, ty2 r) \
        { \
                return (name) { \
                        .l = l, \
                        .r = r, \
                }; \
        }

#endif // PAIR_H_INCLUDED
