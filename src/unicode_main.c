#define BUILD_CONSOLE_INTERFACE 1

#include "krueger_base.h"
#include "krueger_platform.h"

#include "krueger_base.c"
#include "krueger_platform.c"

global const u8 utf8_class[32] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5,
};

typedef struct {
  u32 size;
  u32 codepoint;
} Unicode_Decode;

internal Unicode_Decode
utf8_decode(u8 *str, uxx max) {
  Unicode_Decode result = {1, u32_max};
  u8 byte = str[0];
  u8 byte_class = utf8_class[byte >> 3];
  switch (byte_class) {
    case 1: {
      result.codepoint = byte;
    } break;
    case 2: {
      if (1 < max) {
        u8 count_byte = str[1];
        if (utf8_class[count_byte >> 3] == 0) {
          result.codepoint = ((byte & bitmask5) << 6);
          result.codepoint |= (count_byte & bitmask6);
          result.size = 2;
        }
      }
    } break;
    case 3: {
      if (2 < max) {
        u8 count_byte[2] = {str[1], str[2]};
        if (utf8_class[count_byte[0] >> 3] == 0 &&
          utf8_class[count_byte[1] >> 3] == 0) {
          result.codepoint = ((byte & bitmask4) << 12);
          result.codepoint |= ((count_byte[0] & bitmask6) << 6);
          result.codepoint |= (count_byte[1] & bitmask6);
          result.size = 3;
        }
      }
    } break;
    case 4: {
      if (3 < max) {
        u8 count_byte[3] = {str[1], str[2], str[3]};
        if (utf8_class[count_byte[0] >> 3] == 0 &&
          utf8_class[count_byte[1] >> 3] == 0 &&
          utf8_class[count_byte[2] >> 3] == 0) {
          result.codepoint = ((byte & bitmask3) << 18);
          result.codepoint |= ((count_byte[0] & bitmask6) << 12);
          result.codepoint |= ((count_byte[1] & bitmask6) << 6);
          result.codepoint |= (count_byte[2] & bitmask6);
          result.size = 4;
        }
      }
    } break;
  }
  return(result);
}

internal void
entry_point(int argc, char **argv) {
#if 0
  // u8 str[] = {0x41};               // U+0041
  // u8 str[] = {0xCE, 0xBB};         // U+03BB
  // u8 str[] = {0xE3, 0x81, 0x8B};   // U+304B
  // u8 str[] = {0xE2, 0x9B, 0xA7};   // U+26E7
  u8 str[] = {0xF0, 0xB2, 0x8E, 0xAF};   // U+323AF
  Unicode_Decode decoding = utf8_decode(str, array_count(str));
  log_info("[%d] 0x%X", decoding.size, decoding.codepoint);
#endif
  // u8 str[] = {
  //   0xE3, 0x81, 0x8B, // NOTE: か U+304B
  //   0xE3, 0x81, 0x9A, // NOTE: ず U+305A
  //   0xE3, 0x81, 0x8A, // NOTE: お U+304A
  // };
  u8 *str = "かずお";
  u8 *ptr = str;
  u8 *opl = str + cstr_len(str);
  for (;ptr != opl;) {
    Unicode_Decode decode = utf8_decode(ptr, (opl - ptr));
    log_info("[%d] 0x%X", decode.size, decode.codepoint);
    ptr += decode.size;
  }
}
