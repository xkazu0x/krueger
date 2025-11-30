#define PLATFORM_FEATURE_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_shared.h"

#include "krueger_base.c"
#include "krueger_platform.c"

typedef struct {
  Platform_Handle h;
  #define PROC(x) x##_proc *x;
  KRUEGER_PROC_LIST
  #undef PROC
} Library;

internal Library
libkrueger_load(String8 dst_path, String8 src_path) {
  Library lib = {0};
  if (platform_copy_file_path(dst_path, src_path)) {
    lib.h = platform_library_open(dst_path);
    if (!platform_handle_is_null(lib.h)) {
      #define PROC(x) \
        lib.x = (x##_proc *)platform_library_load_proc(lib.h, #x); \
        if (!(lib.x)) log_error("%s: failed to load proc: [%s]", __func__, #x);
      KRUEGER_PROC_LIST
      #undef PROC
    } else {
      log_error("%s: failed to open library: [%s]", __func__, dst_path.str);
    }
  } else {
    log_error("%s: failed to copy file path: from [%s] to [%s]", __func__, src_path.str, dst_path.str);
  }
  return(lib);
}

internal void
libkrueger_unload(Library lib) {
  if (!platform_handle_is_null(lib.h)) {
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
memory_alloc(uxx size) {
  Memory result = {0};
  result.size = size;
  result.ptr = platform_reserve(size);
  platform_commit(result.ptr, size);
  return(result);
}

internal void
entry_point(int argc, char **argv) {
  Arena *arena = arena_alloc();
  String8 exec_file_path = platform_get_exec_file_path(arena);

#if PLATFORM_WINDOWS
  uxx last_slash_index = str8_find_last(exec_file_path, '\\');
  String8 src_lib_name = str8_lit("libkrueger.dll");
  String8 dst_lib_name = str8_lit("libkruegerx.dll");
#elif PLATFORM_LINUX
  uxx last_slash_index = str8_find_last(exec_file_path, '/');
  String8 src_lib_name = str8_lit("libkrueger.so");
  String8 dst_lib_name = str8_lit("libkruegerx.so");
#endif

  String8 exec_path = str8_substr(exec_file_path, 0, last_slash_index + 1);
  String8 src_lib_path = str8_cat(arena, exec_path, src_lib_name);
  String8 dst_lib_path = str8_cat(arena, exec_path, dst_lib_name);

  Library lib = libkrueger_load(dst_lib_path, src_lib_path);
  if (!platform_handle_is_null(lib.h)) {
    Krueger_Config config = {0};
    config.render_w = 800;
    config.render_h = 600;
    config.window_w = config.render_w;
    config.window_h = config.render_h;
    config.window_title = str8_lit("krueger");
    lib.krueger_config(&config);

    Memory memory = memory_alloc(GB(1));
    Thread_Context *thread_context = thread_context_selected();

    lib.krueger_init(thread_context, &memory, config);

    Image back_buffer = image_alloc(config.render_w, config.render_h);
    Platform_Handle window = platform_window_open(config.window_title,
                                                  config.window_w,
                                                  config.window_h);
    platform_window_toggle_fullscreen(window);
    platform_window_show(window);

    Platform_Graphics_Info graphics_info = platform_get_graphics_info();
    Clock time = { .dt_sec = 1.0f/graphics_info.refresh_rate };
    // Clock time = { .dt_sec = 1.0f/60.0f };
    // Clock time = { .dt_sec = 1.0f/30.0f };
    Input input = {0};

    u64 time_start = platform_get_time_us();

    for (b32 quit = false; !quit;) {
      Temp scratch = scratch_begin(0, 0);
      Platform_Event_List event_list = platform_get_event_list(scratch.arena);
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
      if (quit) break;

#if BUILD_DEBUG
      if (input.kbd[KEY_R].pressed) {
        libkrueger_unload(lib);
        lib = libkrueger_load(dst_lib_path, src_lib_path);
      }
#endif

      if (input.kbd[KEY_F11].pressed) platform_window_toggle_fullscreen(window);
      if (lib.krueger_frame) {
        if (lib.krueger_frame(thread_context, &memory, &back_buffer, &input, &time)) {
          break;
        }
      }
      wait_to_flip(time.dt_sec, time_start);

      u64 time_end = platform_get_time_us();
      time._dt_us = time_end - time_start;
      time._dt_ms = time._dt_us/thousand(1.0f);
      time_start = time_end;

      platform_window_display_buffer(window,
                                     back_buffer.pixels,
                                     back_buffer.width,
                                     back_buffer.height);
      input_reset(&input);
      scratch_end(scratch);
    }

    platform_window_close(window);
    libkrueger_unload(lib);
  }
}
