#ifndef KRUEGER_BASE_STRING_C
#define KRUEGER_BASE_STRING_C

//////////////////////////
// NOTE: Character Helpers

internal b32
char_is_slash(u8 c) {
  b32 result = (c == '/') || (c == '\\');
  return(result);
}

/////////////////////////
// NOTE: C-String Helpers

internal uxx
cstr_len(char *cstr) {
  char *ptr = cstr;
  for (; *ptr != 0; ++ptr);
  return(ptr - cstr);
}

internal uxx
cstr_index_of(char *cstr, char c) {
  uxx result = 0;
  uxx len = cstr_len(cstr);
  for (uxx i = 0; i < len; ++i) {
    if (cstr[i] == c) {
      result = i;
      break;
    }
  }
  return(result);
}

internal b32
cstr_match(char *a, char *b) {
  b32 result = false;
  uxx a_len = cstr_len(a);
  uxx b_len = cstr_len(b);
  if (a_len == b_len) {
    result = true;
    uxx len = min(a_len, b_len);
    for (uxx i = 0; i < len; ++i) {
      char at = a[i];
      char bt = b[i];
      if (at != bt) {
        result = false;
        break;
      }
    }
  }
  return(result);
}

internal uxx
cstr_encode(char *cstr) {
  uxx result = 0;
  uxx len = cstr_len(cstr);
  for (uxx i = 0; i < len; ++i) {
    result |= cstr[i] << i*8;
  }
  return(result);
}

////////////////////////////
// NOTE: String Constructors

internal String8
make_str8(u8 *str, u32 len) {
  String8 result = {
    .len = len,
    .str = str,
  };
  return(result);
}

internal String8
str8_range(u8 *first, u8 *last) {
  String8 result = {
    .len = last - first,
    .str = first,
  };
  return(result);
}

internal String8
str8_cstr(char *cstr) {
  String8 result = {
    .len = cstr_len(cstr),
    .str = (u8 *)cstr,
  };
  return(result);
}

//////////////////////////
// NOTE: String Formatting

internal String8
str8_cat(Arena *arena, String8 a, String8 b) {
  String8 result;
  result.len = a.len + b.len,
  result.str = push_array(arena, u8, result.len + 1);
  memmove(result.str, a.str, a.len);
  memmove(result.str + a.len, b.str, b.len);
  result.str[result.len] = 0;
  return(result);
}

#endif // KRUEGER_BASE_STRING_C
