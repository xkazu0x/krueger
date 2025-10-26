#ifndef KRUEGER_BASE_ARRAY_H
#define KRUEGER_BASE_ARRAY_H

#define arr_def(type) \
  typedef struct { \
    uxx cap; \
    uxx len; \
    type *items; \
  } type##_Array;

#define arr_clear(arr) (arr)->len = 0;
#define arr_push(arr, x) \
  do { \
    assert((arr)->len + 1 <= (arr)->cap); \
    (arr)->items[(arr)->len++] = (x); \
  } while (0)

#endif // KRUEGER_BASE_ARRAY_H
