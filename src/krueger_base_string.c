#ifndef KRUEGER_BASE_STRING_Cstring
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
  b32 result = ((c >= 'A') && (c <= 'Z'));
  return(result);
}

internal b32
char_is_lower(u8 c) {
  b32 result = ((c >= 'a') && (c <= 'z'));
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
str8_find_first(String8 string, u8 c) {
  u8 *ptr = string.str;
  for (uxx i = 0; i < string.len; ++i) {
    u8 *str = string.str + i;
    if (*str == c) {
      ptr = str;
      break;
    }
  }
  uxx result = ptr - string.str;
  return(result);
}

internal uxx
str8_find_last(String8 string, u8 c) {
  u8 *ptr = string.str;
  for (uxx i = string.len - 1; i >= 0; --i) {
    u8 *str = string.str + i;
    if (*str == c) {
      ptr = str;
      break;
    }
  }
  uxx result = ptr - string.str;
  return(result);
}

///////////////////////
// NOTE: String Slicing

internal String8
str8_substr(String8 string, uxx min, uxx max) {
  min = clamp_top(min, string.len);
  max = clamp_top(max, string.len);
  string.str += min;
  string.len = max - min;
  return(string);
}

internal String8
str8_skip(String8 string, uxx amt) {
  amt = clamp_top(amt, string.len);
  string.str += amt;
  string.len -= amt;
  return(string);
}

internal String8
str8_chop(String8 string, uxx amt) {
  amt = clamp_top(amt, string.len);
  string.len -= amt;
  return(string);
}

internal String8
str8_prefix(String8 string, uxx size) {
  size = clamp_top(size, string.len);
  string.len = size;
  return(string);
}

internal String8
str8_postfix(String8 string, uxx size) {
  size = clamp_top(size, string.len);
  string.str += string.len - size;
  string.len = size;
  return(string);
}

//////////////////////////
// NOTE: String Formatting

internal String8
str8_cat(Arena *arena, String8 a, String8 b) {
  String8 result;
  result.len = a.len + b.len,
  result.str = push_array(arena, u8, result.len + 1);
  mem_copy(result.str, a.str, a.len);
  mem_copy(result.str + a.len, b.str, b.len);
  result.str[result.len] = 0;
  return(result);
}

internal String8
str8_copy(Arena *arena, String8 string) {
  String8 result = {0};
  result.len = string.len;
  result.str = push_array(arena, u8, result.len + 1);
  mem_copy(result.str, string.str, string.len);
  result.str[result.len] = 0;
  return(result);
}

internal String8
str8_fmt_args(Arena *arena, char *fmt, va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);
  u32 size = vsnprintf(0, 0, fmt, args) + 1;
  String8 result = {0};
  result.str = push_array(arena, u8, size);
  result.len = vsnprintf((char *)result.str, size, fmt, args_copy);
  result.str[result.len] = 0;
  va_end(args_copy);
  return(result);
}

internal String8
str8_fmt(Arena *arena, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  String8 result = str8_fmt_args(arena, fmt, args);
  va_end(args);
  return(result);
}

////////////////////
// NOTE: String List

internal String8_Node *
str8_list_push_node(String8_List *list, String8_Node *node) {
  queue_push(list->first, list->last, node);
  list->count += 1;
  list->size += node->string.len;
  return(node);
}

internal String8_Node *
str8_list_push_node_and_set_string(String8_List *list, String8_Node *node, String8 string) {
  node->string = string;
  queue_push(list->first, list->last, node);
  list->count += 1;
  list->size += string.len;
  return(node);
}

internal String8_Node *
str8_list_push(Arena *arena, String8_List *list, String8 string) {
  String8_Node *node = push_array(arena, String8_Node, 1);
  str8_list_push_node_and_set_string(list, node, string);
  return(node);
}

internal String8_Node *
str8_list_push_copy(Arena *arena, String8_List *list, String8 string) {
  String8 copy = str8_copy(arena, string);
  String8_Node *result = str8_list_push(arena, list, copy);
  return(result);
}

internal String8_Node *
str8_list_push_fmt(Arena *arena, String8_List *list, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  String8 string = str8_fmt_args(arena, fmt, args);
  String8_Node *result = str8_list_push(arena, list, string);
  va_end(args);
  return(result);
}

internal String8
str8_list_join(Arena *arena, String8_List *list) {
  String8 result = {0};
  result.len = list->size;
  u8 *ptr = result.str = push_array(arena, u8, result.len + 1);
  for (String8_Node *node = list->first; node != 0; node = node->next) {
    mem_copy(ptr, node->string.str, node->string.len);
    ptr += node->string.len;
  }
  *ptr = 0;
  return(result);
}

////////////////////////////
// NOTE: String Path Helpers

internal String8
str8_chop_last_slash(String8 string) {
  if (string.len > 0) {
    u8 *ptr = string.str + string.len - 1;
    for (; ptr >= string.str; ptr -= 1) {
      if (*ptr == '/' || *ptr == '\\') {
        break;
      }
    }
    if (ptr >= string.str) {
      string.len = ptr - string.str;
    } else {
      string.len = 0;
    }
  }
  return(string);
}

#endif // KRUEGER_BASE_STRING_C
