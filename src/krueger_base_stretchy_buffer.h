#ifndef KRUEGER_BASE_STRETCHY_BUFFER_H
#define KRUEGER_BASE_STRETCHY_BUFFER_H

typedef struct {
  uxx len;
  uxx cap;
  u8 *ptr[0];
} Stretchy_Buffer_Header;

#define sb__hdr(b) ((Stretchy_Buffer_Header *)((u8 *)(b) - offsetof(Stretchy_Buffer_Header, ptr)))
#define sb__fits(b, n) (sb_len(b) + (n) <= sb_cap(b))
#define sb__fit(b, n) (sb__fits((b), (n)) ? 0 : ((b) = sb__grow((b), sb_len(b) + (n), sizeof(*(b)))))

#define sb_len(b) ((b) ? sb__hdr(b)->len : 0)
#define sb_cap(b) ((b) ? sb__hdr(b)->cap : 0)
#define sb_push(b, x) (sb__fit((b), 1), (b)[sb__hdr(b)->len++] = (x))
#define sb_free(b) ((b) ? (free(sb__hdr(b)), (b) = 0) : 0)
#define sb_clear(b) ((b) ? sb__hdr(b)->len = 0 : 0)

internal void *sb__grow(void *buf, uxx count, uxx type_size);

#endif // KRUEGER_BASE_STRETCHY_BUFFER_H
