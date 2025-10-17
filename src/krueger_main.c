// NOTE: platform features
#define PLATFORM_GFX 1

// NOTE: [h]
#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_shared.h"

// NOTE: [c]
#include "krueger_base.c"
#include "krueger_platform.c"

// NOTE: constants
#define WINDOW_TITLE "krueger"
#define WINDOW_SCALE  3

#define BACK_BUFFER_WIDTH  320
#define BACK_BUFFER_HEIGHT 240

#define WINDOW_WIDTH  WINDOW_SCALE*BACK_BUFFER_WIDTH
#define WINDOW_HEIGHT WINDOW_SCALE*BACK_BUFFER_HEIGHT

#include <stdio.h>

typedef struct {
  Platform_Handle h;
  #define PROC(x) x##_proc *x;
  KRUEGER_PROC_LIST
  #undef PROC
} Library;

internal Library
libkrueger_load(String8 dst_path, String8 src_path) {
  Library lib = {0};
  if (platform_copy_file_path((char *)dst_path.str, (char *)src_path.str)) {
    lib.h = platform_library_open((char *)dst_path.str);
    if (!platform_handle_match(lib.h, PLATFORM_HANDLE_NULL)) {
      #define PROC(x) \
        lib.x = (x##_proc *)platform_library_load_proc(lib.h, #x); \
        if (!(lib.x)) printf("[ERROR]: %s: failed to load proc from library [%s]: proc [%s]\n", __func__, dst_path.str, #x);
      KRUEGER_PROC_LIST
      #undef PROC
    } else {
      printf("[ERROR]: %s: failed to open library: [%s]\n", __func__, dst_path.str);
    }
  } else {
    printf("[ERROR]: %s: failed to copy file path: from [%s] to [%s]\n", __func__, src_path.str, dst_path.str);
  }
  return(lib);
}

internal void
libkrueger_unload(Library lib) {
  if (!platform_handle_match(lib.h, PLATFORM_HANDLE_NULL)) {
    platform_library_close(lib.h);
  }
}

internal void
process_digital_button(Digital_Button *b, b32 is_down) {
  b32 was_down = b->is_down;
  b->pressed = !was_down && is_down; 
  b->released = was_down && !is_down;
  b->is_down = is_down;
}

internal void
input_reset(Input *input) {
  for (u32 key = 0; key < KEY_MAX; ++key) {
    input->kbd[key].pressed = false;
    input->kbd[key].released = false;
  }
}

int
main(void) {
  platform_init_core();

  Arena str_arena = arena_alloc(MB(1));
  String8 exec_file_path = platform_get_exec_file_path(&str_arena);

  u8 *exec_file_name = exec_file_path.str;
  for (u8 *scan = exec_file_path.str; *scan; ++scan) {
    if (char_is_slash(*scan)) {
      exec_file_name = scan + 1;
    }
  }

  String8 path = str8_range(exec_file_path.str, exec_file_name);

#if PLATFORM_WINDOWS
  String8 src_lib_name = str8_lit("libkrueger.dll");
  String8 dst_lib_name = str8_lit("libkruegerx.dll");
#elif PLATFORM_LINUX
  String8 src_lib_name = str8_lit("libkrueger.so");
  String8 dst_lib_name = str8_lit("libkruegerx.so");
#endif

  String8 src_lib_path = str8_cat(&str_arena, path, src_lib_name);
  String8 dst_lib_path = str8_cat(&str_arena, path, dst_lib_name);
  
  Library lib = libkrueger_load(dst_lib_path, src_lib_path);
  if (!platform_handle_match(lib.h, PLATFORM_HANDLE_NULL)) {
    Krueger_State *krueger_state = 0;
    if (lib.krueger_init) krueger_state = lib.krueger_init();

    Image back_buffer = image_alloc(BACK_BUFFER_WIDTH, BACK_BUFFER_HEIGHT);
    Input input = {0};
    Clock time = {0};

    platform_create_window(&(Platform_Window_Desc){
      .window_title = "krueger",
      .window_w = WINDOW_WIDTH,
      .window_h = WINDOW_HEIGHT,
      .back_buffer_w = back_buffer.width,
      .back_buffer_h = back_buffer.height,
    });

    u64 time_start = platform_get_time_us();

    for (b32 quit = false; !quit;) {
      platform_update_window_events();
      for (uxx i = 0; i < buf_len(platform_event_buf); ++i) {
        Platform_Event event = platform_event_buf[i];
        switch (event.type) {
          case PLATFORM_EVENT_QUIT: {
            quit = true;
          } break;
          case PLATFORM_EVENT_KEY_PRESS:
          case PLATFORM_EVENT_KEY_RELEASE: {
            Keycode keycode = event.keycode;
            b32 is_down = (event.type == PLATFORM_EVENT_KEY_PRESS);
            process_digital_button(input.kbd + keycode, is_down);
          } break;
        }
      }

      if (input.kbd[KEY_Q].pressed) {
        quit = true;
      }

      if (input.kbd[KEY_R].pressed) {
        libkrueger_unload(lib);
        lib = libkrueger_load(dst_lib_path, src_lib_path);
      }

      if (lib.krueger_frame) lib.krueger_frame(krueger_state, back_buffer, input, time);
      platform_display_back_buffer(back_buffer.pixels, back_buffer.width, back_buffer.height);

      u64 time_end = platform_get_time_us();
      time.dt_us = (f32)(time_end - time_start);
      time.dt_ms = time.dt_us/thousand(1);
      time.dt_sec = time.dt_ms/thousand(1);
      time.us += time.dt_us;
      time.ms += time.dt_ms;
      time.sec += time.dt_sec;
      time_start = time_end;

      input_reset(&input);
    }
    platform_destroy_window();
    libkrueger_unload(lib);
  }
  return(0);
}
