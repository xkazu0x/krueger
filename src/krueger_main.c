#define KRUEGER_PLATFORM_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#include "krueger_shared.h"

#define WINDOW_TITLE str8_lit("STARFIGHTER")
#define WINDOW_SCALE 5

#define BACK_BUFFER_WIDTH  128
#define BACK_BUFFER_HEIGHT 128

#define WINDOW_WIDTH  WINDOW_SCALE*BACK_BUFFER_WIDTH
#define WINDOW_HEIGHT WINDOW_SCALE*BACK_BUFFER_HEIGHT

#define PATH_SLASH_CHAR ((PLATFORM_WINDOWS) ? '\\' : '/')

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
        if (!(lib.x)) log_error("%s: failed to load proc: [%s]", __func__, #x);
      KRUEGER_PROC_LIST
      #undef PROC
    } else {
      log_error("%s: failed to open library: [%s]", __func__, dst_path.str);
    }
  } else {
    log_error("%s: failed to copy file path from [%s] to [%s]", __func__, src_path.str, dst_path.str);
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

internal f32
get_seconds_elapsed(u64 start, u64 end) {
  f32 result = (end - start)/million(1.0f);
  return(result);
}

internal void
wait_to_flip(f32 target_sec_per_frame, u64 time_start) {
  u64 time_end = platform_get_time_us();
  f32 sec_per_frame = get_seconds_elapsed(time_start, time_end);
  if (sec_per_frame < target_sec_per_frame) {
    u32 sleep_ms = (u32)((target_sec_per_frame - sec_per_frame)*thousand(1.0f));
    if (sleep_ms > 0) platform_sleep_ms(sleep_ms);
    while (sec_per_frame < target_sec_per_frame) {
      time_end = platform_get_time_us();
      sec_per_frame = get_seconds_elapsed(time_start, time_end);
    }
  }
}

internal Memory
memory_alloc(uxx memory_size) {
  u8 *memory_ptr = platform_reserve(memory_size);
  platform_commit(memory_ptr, memory_size);
  Memory result = {
    .memory_size = memory_size,
    .memory_ptr = memory_ptr,
  };
  return(result);
}

internal Image
image_alloc(u32 width, u32 height) {
  uxx size = width*height*sizeof(u32);
  u32 *pixels = platform_reserve(size);
  platform_commit(pixels, size);
  mem_zero(pixels, size);
  Image result = make_image(pixels, width, height);
  return(result);
}

int
main(void) {
  platform_core_init();
  platform_graphics_init();

  Arena misc_arena = arena_alloc(MB(64));
  String8 exec_file_path = platform_get_exec_file_path(&misc_arena);
  uxx last_slash_index = str8_index_of_last(exec_file_path, PATH_SLASH_CHAR);
  String8 path = str8_substr(exec_file_path, 0, last_slash_index + 1);

#if PLATFORM_WINDOWS
  String8 src_lib_name = str8_lit("libkrueger.dll");
  String8 dst_lib_name = str8_lit("libkruegerx.dll");
#elif PLATFORM_LINUX
  String8 src_lib_name = str8_lit("libkrueger.so");
  String8 dst_lib_name = str8_lit("libkruegerx.so");
#endif

  String8 src_lib_path = str8_cat(&misc_arena, path, src_lib_name);
  String8 dst_lib_path = str8_cat(&misc_arena, path, dst_lib_name);

  Library lib = libkrueger_load(dst_lib_path, src_lib_path);
  if (!platform_handle_match(lib.h, PLATFORM_HANDLE_NULL)) {
    Platform_Handle window = platform_window_open(
      WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT
    );

    Memory memory = memory_alloc(GB(1));
    Image back_buffer = image_alloc(BACK_BUFFER_WIDTH, BACK_BUFFER_HEIGHT);
    Input input = {0};
    Clock time = {0};

#if 1
    Platform_Graphics_Info graphics_info = platform_get_graphics_info();
    time.dt_sec = 1.0f/graphics_info.refresh_rate;
#else
    time.dt_sec = 1.0f/30.0f;
#endif

    if (lib.krueger_init) lib.krueger_init(&memory, &back_buffer);
    u64 time_start = platform_get_time_us();
  
    platform_window_show(window);
    for (b32 quit = false; !quit;) {
      Temp temp = temp_begin(&misc_arena);
      Platform_Event_List event_list = platform_get_event_list(temp.arena);
      for (Platform_Event *event = event_list.first; event != 0; event = event->next) {
        switch (event->type) {
          case PLATFORM_EVENT_WINDOW_CLOSE: {
            quit = true;
          } break;
          case PLATFORM_EVENT_KEY_PRESS:
          case PLATFORM_EVENT_KEY_RELEASE: {
            Keycode keycode = event->keycode;
            b32 is_down = (event->type == PLATFORM_EVENT_KEY_PRESS);
            process_digital_button(input.kbd + keycode, is_down);
          } break;
        }
      }

      if (input.kbd[KEY_Q].pressed) quit = true;
      if (input.kbd[KEY_F11].pressed) platform_window_toggle_fullscreen(window);

      if (input.kbd[KEY_F4].pressed) {
        libkrueger_unload(lib);
        lib = libkrueger_load(dst_lib_path, src_lib_path);
      }

      if (lib.krueger_frame) lib.krueger_frame(&memory, &back_buffer, &input, &time);
      wait_to_flip(time.dt_sec, time_start);

      u64 time_end = platform_get_time_us();
      time._dt_us = time_end - time_start;
      time._dt_ms = time._dt_us/thousand(1.0f);
      time_start = time_end;

      platform_window_display_buffer(
        window, back_buffer.pixels, back_buffer.width, back_buffer.height
      );
      input_reset(&input);
    }

    platform_window_close(window);
    libkrueger_unload(lib);
  }
  platform_core_shutdown();
  return(0);
}
