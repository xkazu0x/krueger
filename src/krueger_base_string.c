#ifndef KRUEGER_BASE_STRING_C
#define KRUEGER_BASE_STRING_C

//////////////////////////
// NOTE: Character Helpers

internal b32
char_is_space(u8 c) {
  b32 result = ((c == ' ') || (c == '\r') || (c == '\n') || (c == '\t') || (c == '\v') || (c == '\f'));
  return(result);
}

internal b32
char_is_upper(u8 c) {
  b32 result = ((c >= 'a') && (c <= 'z'));
  return(result);
}

internal b32
char_is_lower(u8 c) {
  b32 result = ((c >= 'A') && (c <= 'Z'));
  return(result);
}

internal b32
char_is_alpha(u8 c) {
  b32 result = (char_is_upper(c) || char_is_lower(c));
  return(result);
}

internal b32
char_is_digit(u8 c) {
  b32 result = ((c >= '0') && (c <= '9'));
  return(result);
}

internal b32
char_is_slash(u8 c) {
  b32 result = ((c == '/') || (c == '\\'));
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
make_str8(u8 *str, uxx len) {
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

////////////////////////
// NOTE: String Matching

internal b32
str8_match(String8 a, String8 b) {
  b32 result = false;
  if (a.len == b.len) {
    result = true;
    uxx len = min(a.len, b.len);
    for (uxx i = 0; i < len; ++i) {
      u8 at = a.str[i];
      u8 bt = b.str[i];
      if (at != bt) {
        result = false;
        break;
      }
    }
  }
  return(result);
}

internal uxx
str8_index_of_last(String8 str, u8 c) {
  u8 *tmp_ptr = str.str;
  for (uxx i = 0; i < str.len; ++i) {
    u8 *str_ptr = str.str + i;
    if (*str_ptr == c) {
      tmp_ptr = str_ptr;
    }
  }
  uxx result = tmp_ptr - str.str;
  return(result);
}

///////////////////////
// NOTE: String Slicing

internal String8
str8_substr(String8 str, uxx min, uxx max) {
  min = clamp_top(min, str.len);
  max = clamp_top(max, str.len);
  str.str += min;
  str.len = max - min;
  return(str);
}

internal String8
str8_skip(String8 str, uxx amt) {
  amt = clamp_top(amt, str.len);
  str.str += amt;
  str.len -= amt;
  return(str);
}

internal String8
str8_chop(String8 str, uxx amt) {
  amt = clamp_top(amt, str.len);
  str.len -= amt;
  return(str);
}

//////////////////////////
// NOTE: String Formatting

internal String8
str8_cat(Arena *arena, String8 a, String8 b) {
  String8 result;
  result.len = a.len + b.len,
  result.str = push_array(arena, u8, result.len + 1);
  mem_cpy(result.str, a.str, a.len);
  mem_cpy(result.str + a.len, b.str, b.len);
  result.str[result.len] = 0;
  return(result);
}

internal String8
str8_fmt(Arena *arena, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  u32 size = vsnprintf(0, 0, fmt, args) + 1;
  String8 result = {0};
  result.str = push_array(arena, u8, size);
  result.len = vsnprintf((char *)result.str, size, fmt, args);
  result.str[result.len] = 0;
  va_end(args);
  return(result);
}

////////////////////////

internal u32
u32_from_str8(String8 str) {
  u32 result = 0;
  for (uxx i = 0; i < str.len; ++i) {
    u8 c = str.str[i];
    if (char_is_digit(c)) {
      result *= 10;
      result += c - '0';
    }
  }
  return(result);
}

internal f32
f32_from_str8(String8 str) {
  // TODO: This may cause overflow.
  f32 result = 0.0f;
  f32 sign = 1.0f;
  if (str.str[0] == '-') sign = -1.0f;
  for (uxx i = 0; i < str.len; ++i) {
    u8 c = str.str[i];
    if (char_is_digit(c)) {
      result *= 10.0f;
      result += c - '0';
    }
  }
  result = sign*result/million(1.0f);
  return(result);
}

#endif // KRUEGER_BASE_STRING_C
