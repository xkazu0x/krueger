#ifndef KRUEGER_BASE_STRETCHY_BUFFER_C
#define KRUEGER_BASE_STRETCHY_BUFFER_C

internal void *
sb__grow(void *buf, uxx count, uxx type_size) {
  uxx cap = max(1 + 2*sb_cap(buf), count);
  uxx size = offsetof(Stretchy_Buffer_Header, ptr) + cap*type_size;
  Stretchy_Buffer_Header *hdr = realloc((buf) ? sb__hdr(buf) : 0, size);
  if (!buf) hdr->len = 0;
  hdr->cap = cap;
  return(hdr->ptr);
}

#endif // KRUEGER_BASE_STRETCHY_BUFFER_C
