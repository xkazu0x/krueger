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

///////////////////////////////////
// NOTE: String Splitting & Joining

internal String8_List
str8_split(Arena *arena, String8 string, u8 *splits, u32 num_splits) {
  String8_List result = {0};

  u8 *at = string.str;
  u8 *stop = string.str + string.len;

  u8 *str_start = at;
  u8 *str_end = stop;

  for (; at != stop; at += 1) {
    for (u32 split_index = 0;
         split_index < num_splits;
         ++split_index) {
      if (*at == splits[split_index]) {
        str_end = at;
        break;
      }
    }

    if (str_end == at) {
      str8_list_push(arena, &result, str8_range(str_start, str_end));
      str_start = str_end + 1;
    }
  }

  return(result);
}

internal String8
str8_list_join(Arena *arena, String8_List *list, String_Join *opt_join) {
  String_Join join = {0};
  if (opt_join != 0) {
    mem_copy_struct(&join, opt_join);
  }

  u32 sep_count = 0;
  if (list->count > 0) {
    sep_count = list->count - 1;
  }

  String8 result = {0};
  result.len = list->size + join.pre.len + sep_count*join.sep.len + join.post.len;
  u8 *ptr = result.str = push_array(arena, u8, result.len + 1);

  mem_copy(ptr, join.pre.str, join.pre.len);
  ptr += join.pre.len;
  for (String8_Node *node = list->first; node != 0; node = node->next) {
    mem_copy(ptr, node->string.str, node->string.len);
    ptr += node->string.len;
    if (node->next != 0) {
      mem_copy(ptr, join.sep.str, join.sep.len);
      ptr += join.sep.len;
    }
  }
  mem_copy(ptr, join.post.str, join.post.len);
  ptr += join.post.len;
  *ptr = 0;

  return(result);
}

////////////////////////////
// NOTE: String Path Helpers

internal String8_List
str8_split_path(Arena *arena, String8 path) {
  String8_List result = str8_split(arena, path, (u8 *)"/\\", 2);
  return(result);
}

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
