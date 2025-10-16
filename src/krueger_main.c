// NOTE: platform options
#define PLATFORM_GFX 1

// NOTE: [h]
#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_shared.h"

// NOTE: [c]
#include "krueger_base.c"
#include "krueger_platform.c"

#define BACK_BUFFER_WIDTH  320
#define BACK_BUFFER_HEIGHT 240

#define WINDOW_SCALE  3
#define WINDOW_WIDTH  WINDOW_SCALE*BACK_BUFFER_WIDTH
#define WINDOW_HEIGHT WINDOW_SCALE*BACK_BUFFER_HEIGHT

#include <stdio.h>

global Platform_Handle libkrueger;
#define PROC(x) global x##_proc *x;
KRUEGER_PROC_LIST
#undef PROC

internal void
libkrueger_reload(char *dst_path, char *src_path) {
  if (!platform_handle_match(libkrueger, PLATFORM_HANDLE_NULL)) {
    platform_library_close(libkrueger);
  }
  if (platform_copy_file_path(dst_path, src_path)) {
    libkrueger = platform_library_open(dst_path);
    if (!platform_handle_match(libkrueger, PLATFORM_HANDLE_NULL)) {
      #define PROC(x) \
        x = (x##_proc *)platform_library_load_proc(libkrueger, #x); \
        if (!(x)) printf("[ERROR]: reload_libkrueger: failed to load proc from library %s: %s\n", dst_path, #x);
      KRUEGER_PROC_LIST
      #undef PROC
    } else {
      printf("[ERROR]: libkrueger_load: failed to open library: %s\n", dst_path);
    }
  } else {
    printf("[ERROR]: libkrueger_load: failed to copy file path: from [%s] to [%s]\n", src_path, dst_path);
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

  char *src_lib_path = "..\\build\\libkrueger.dll";
  char *dst_lib_path = "..\\build\\libkruegerx.dll";
  libkrueger_reload(dst_lib_path, src_lib_path);

  Krueger_State *krueger_state = 0;
  if (krueger_init) krueger_state = krueger_init();

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

    if (input.kbd[KEY_Q].pressed) quit = true;
    if (input.kbd[KEY_R].pressed) libkrueger_reload(dst_lib_path, src_lib_path);

    if (krueger_frame) krueger_frame(krueger_state, back_buffer, input, time);
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
  return(0);
}
