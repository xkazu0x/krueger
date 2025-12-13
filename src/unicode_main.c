#define BUILD_CONSOLE_INTERFACE 1

#include "krueger_base.h"
#include "krueger_platform.h"

#include "krueger_base.c"
#include "krueger_platform.c"

global const u8 utf8_class[32] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5,
};

typedef struct {
  u32 inc;
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
          result.inc = 2;
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
          result.inc = 3;
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
          result.inc = 4;
        }
      }
    } break;
  }
  return(result);
}

internal u32
utf8_encode(u8 *str, u32 codepoint) {
  u32 inc = 0;
  if (codepoint <= 0x7F) {
    str[0] = (u8)codepoint;
    inc = 1;
  } else if (codepoint <= 0x7FF) {
    str[0] = (u8)((bitmask2 << 6) | ((codepoint >> 6) & bitmask5));
    str[1] = (u8)(bit8 | (codepoint & bitmask6));
    inc = 2;
  } else if (codepoint <= 0xFFFF) {
    str[0] = (u8)((bitmask3 << 5) | ((codepoint >> 12) & bitmask4));
    str[1] = (u8)(bit8 | ((codepoint >> 6) & bitmask6));
    str[2] = (u8)(bit8 | (codepoint & bitmask6));
    inc = 3;
  } else if (codepoint <= 0x10FFFF) {
    str[0] = (u8)((bitmask4 << 4) | ((codepoint >> 18) & bitmask3));
    str[1] = (u8)(bit8 | ((codepoint >> 12) & bitmask6));
    str[2] = (u8)(bit8 | ((codepoint >> 6) & bitmask6));
    str[3] = (u8)(bit8 | (codepoint & bitmask6));
    inc = 4;
  } else {
    str[0] = '?';
    inc = 1;
  }
  return(inc);
}

internal void
entry_point(int argc, char **argv) {
  u8 str[] = {
    0xE3, 0x81, 0x8B, // NOTE: か U+304B
    0xE3, 0x81, 0x9A, // NOTE: ず U+305A
    0xE3, 0x81, 0x8A, // NOTE: お U+304A
  };

  printf("[sample]:\n");
  for (u32 i = 0; i < array_count(str); i += 3) {
    printf("0x%X 0x%X 0x%X\n", str[i+0], str[i+1], str[i+2]);
  }

  printf("[decoded]:\n");
  u8 *ptr = str;
  u8 *opl = str + array_count(str);
  for (;ptr != opl;) {
    Unicode_Decode decode = utf8_decode(ptr, (opl - ptr));
    ptr += decode.inc;
    printf("[%d] U+%X -> ", decode.inc, decode.codepoint);
  
    u8 encoded[4];
    u32 inc = utf8_encode(encoded, decode.codepoint);
    for (u32 i = 0; i < inc; ++i) {
      printf("0x%X ", encoded[i]);
      if (i == inc - 1) printf("\n");
    }
  }
}
