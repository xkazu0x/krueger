#ifndef KRUEGER_BASE_STRETCHY_BUFFER_C
#define KRUEGER_BASE_STRETCHY_BUFFER_C

////////////////////////
// NOTE: Stretchy Buffer

internal void *
buf__grow(const void *buf, uxx new_len, uxx elem_size) {
  uxx new_cap = max(1 + 2*buf_cap(buf), new_len);
  uxx new_size = offsetof(Buffer_Header, ptr) + new_cap*elem_size;
  Buffer_Header *header = realloc((buf) ? buf__hdr(buf) : 0, new_size);
  if (!buf) header->len = 0;
  header->cap = new_cap;
  return(header->ptr);
}

#endif // KRUEGER_BASE_STRETCHY_BUFFER_C
