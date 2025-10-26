#ifndef KRUEGER_BASE_STRING_H
#define KRUEGER_BASE_STRING_H

/////////////////////
// NOTE: String Types

typedef struct {
  uxx len;
  u8 *str;
} String8;

//////////////////////////
// NOTE: Character Helpers

internal b32 char_is_space(u8 c);
internal b32 char_is_upper(u8 c);
internal b32 char_is_lower(u8 c);
internal b32 char_is_alpha(u8 c);
internal b32 char_is_digit(u8 c);
internal b32 char_is_slash(u8 c);

/////////////////////////
// NOTE: C-String Helpers

internal uxx cstr_len(char *cstr);
internal uxx cstr_index_of(char *cstr, char c);
internal b32 cstr_match(char *a, char *b);
internal uxx cstr_encode(char *cstr);

////////////////////////////
// NOTE: String Constructors

#define str8_comp(str, len) ((String8){(len), (str)})
#define str8_lit(s) make_str8((u8 *)(s), sizeof(s) - 1)
internal String8 make_str8(u8 *str, uxx len);
internal String8 str8_range(u8 *first, u8 *last);
internal String8 str8_cstr(char *cstr);

////////////////////////
// NOTE: String Matching

#define str8_match_lit(a, b) str8_match((a), str8_lit(b))
#define str8_match_cstr(a, b) str8_match((a), str8_cstr(b))
internal b32 str8_match(String8 a, String8 b);
internal uxx str8_index_of_last(String8 str, u8 c);

///////////////////////
// NOTE: String Slicing

internal String8 str8_substr(String8 str, uxx min, uxx max);
internal String8 str8_skip(String8 str, uxx amt);
internal String8 str8_chop(String8 str, uxx amt);

//////////////////////////
// NOTE: String Formatting

internal String8 str8_cat(Arena *arena, String8 a, String8 b);

#endif // KRUEGER_BASE_STRING_H
