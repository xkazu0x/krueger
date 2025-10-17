#ifndef KRUEGER_BASE_STRETCHY_BUFFER_H
#define KRUEGER_BASE_STRETCHY_BUFFER_H

////////////////////////
// NOTE: Stretchy Buffer

typedef struct {
  uxx len;
  uxx cap;
  u8 *ptr[0];
} Buffer_Header;

#define buf__hdr(b) ((Buffer_Header *)((u8 *)(b) - offsetof(Buffer_Header, ptr)))
#define buf__fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define buf__fit(b, n) (buf__fits((b), (n)) ? 0 : ((b) = buf__grow((b), buf_len(b) + (n), sizeof(*(b)))))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_push(b, x) (buf__fit((b), 1), (b)[buf__hdr(b)->len++] = (x))
#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = 0) : 0)
#define buf_clear(b) ((b) ? buf__hdr(b)->len = 0 : 0)

internal void *buf__grow(const void *buf, uxx new_len, uxx elem_size);

#endif // KRUEGER_BASE_STRETCHY_BUFFER_H
