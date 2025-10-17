#ifndef KRUEGER_BASE_STRING_H
#define KRUEGER_BASE_STRING_H

///////////////
// NOTE: String

typedef struct {
  uxx len;
  u8 *str;
} String8;

//////////////////////////
// NOTE: Character Helpers

internal b32 char_is_slash(u8 c);

/////////////////////////
// NOTE: C-String Helpers

internal uxx cstr_len(char *cstr);
internal uxx cstr_index_of(char *cstr, char c);
internal b32 cstr_match(char *a, char *b);
internal uxx cstr_encode(char *cstr);

////////////////////////////
// NOTE: String Constructors

#define str8_lit(s) make_str8((u8 *)(s), sizeof(s) - 1)

internal String8 make_str8(u8 *str, u32 len);
internal String8 str8_range(u8 *first, u8 *last);
internal String8 str8_cstr(char *cstr);

//////////////////////////
// NOTE: String Formatting

internal String8 str8_cat(Arena *arena, String8 a, String8 b);

#endif // KRUEGER_BASE_STRING_H
