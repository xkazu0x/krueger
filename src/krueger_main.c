#define BUILD_CONSOLE_INTERFACE 1
#define PLATFORM_FEATURE_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_shared.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#include <xinput.h>

#define XINPUT_GET_STATE(x) DWORD x(DWORD dwUserIndex, XINPUT_STATE *pState)
#define XINPUT_SET_STATE(x) DWORD x(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)

typedef XINPUT_GET_STATE(xinput_get_state_proc);
typedef XINPUT_SET_STATE(xinput_set_state_proc);

XINPUT_GET_STATE(xinput_get_state_stub) {
  return(ERROR_DEVICE_NOT_CONNECTED);
}

XINPUT_SET_STATE(xinput_set_state_stub) {
  return(ERROR_DEVICE_NOT_CONNECTED);
}

global xinput_get_state_proc *xinput_get_state = xinput_get_state_stub;
global xinput_set_state_proc *xinput_set_state = xinput_set_state_stub;

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

#define GAMEPAD_MAX 4

typedef struct {
  Digital_Button up;
  Digital_Button down;
  Digital_Button left;
  Digital_Button right;

  Digital_Button start;
  Digital_Button back;

  Digital_Button left_thumb;
  Digital_Button right_thumb;

  Digital_Button left_shoulder;
  Digital_Button right_shoulder;

  Digital_Button a;
  Digital_Button b;
  Digital_Button x;
  Digital_Button y;

  Analog_Button left_trigger;
  Analog_Button right_trigger;

  Stick left_stick;
  Stick right_stick;
} Gamepad;

global Digital_Button keys[KEY_MAX];
global Gamepad gamepads[GAMEPAD_MAX];

internal void
platform_gamepad_init(void) {
  String8 xinput_versions[3];
  xinput_versions[0] = str8_lit("xinput1_4.dll");
  xinput_versions[1] = str8_lit("xinput1_3.dll");
  xinput_versions[2] = str8_lit("xinput9_1_0.dll");

  for (u32 string_index = 0;
       string_index < array_count(xinput_versions);
       ++string_index) {
    String8 string = xinput_versions[string_index];
    Platform_Handle h = platform_library_open(string);
    if (!platform_handle_is_null(h)) {
      xinput_get_state = (xinput_get_state_proc *)
        platform_library_load_proc(h, "XInputGetState");
      xinput_set_state = (xinput_set_state_proc *)
        platform_library_load_proc(h, "XInputSetState");
      break;
    }
  }
}

internal void
platform_gamepad_update(void) {
  u32 gamepad_count = min(GAMEPAD_MAX, XUSER_MAX_COUNT);
  for (u32 gamepad_index = 0;
       gamepad_index < gamepad_count;
       ++gamepad_index) {
    XINPUT_STATE xinput_state;
    if (xinput_get_state(gamepad_index, &xinput_state) == ERROR_SUCCESS) {
      XINPUT_GAMEPAD *xpad = &xinput_state.Gamepad;
      Gamepad *pad = gamepads + gamepad_index;

      process_digital_button(&pad->up, (xpad->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
      process_digital_button(&pad->down, (xpad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
      process_digital_button(&pad->left, (xpad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
      process_digital_button(&pad->right, (xpad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);

      process_digital_button(&pad->start, (xpad->wButtons & XINPUT_GAMEPAD_START) != 0);
      process_digital_button(&pad->back, (xpad->wButtons & XINPUT_GAMEPAD_BACK) != 0);

      process_digital_button(&pad->left_thumb, (xpad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
      process_digital_button(&pad->right_thumb, (xpad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);

      process_digital_button(&pad->left_shoulder, (xpad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
      process_digital_button(&pad->right_shoulder, (xpad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);

      process_digital_button(&pad->a, (xpad->wButtons & XINPUT_GAMEPAD_A) != 0);
      process_digital_button(&pad->b, (xpad->wButtons & XINPUT_GAMEPAD_B) != 0);
      process_digital_button(&pad->x, (xpad->wButtons & XINPUT_GAMEPAD_X) != 0);
      process_digital_button(&pad->y, (xpad->wButtons & XINPUT_GAMEPAD_Y) != 0);

      {
        f32 threshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD/255.0f;
        Analog_Button *button = &pad->left_trigger;
        f32 value = xpad->bLeftTrigger/255.0f;
        b32 was_down = button->is_down;
        button->is_down = (value >= threshold);
        button->pressed = !was_down && button->is_down;
        button->released = was_down && !button->is_down;
      }

      {
        f32 threshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD/255.0f;
        Analog_Button *button = &pad->right_trigger;
        f32 value = xpad->bRightTrigger/255.0f;
        b32 was_down = button->is_down;
        button->is_down = (value >= threshold);
        button->pressed = !was_down && button->is_down;
        button->released = was_down && !button->is_down;
      }

      if (pad->left_trigger.pressed) log_info("[LT] pressed");
      if (pad->right_trigger.pressed) log_info("[RT] pressed");

#define NORMALIZE(x) (2.0f*((x + 32768)/65535.0f) - 1.0f)
      {
        f32 threshold = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/32767.0f;
        f32 x = NORMALIZE(xpad->sThumbLX);
        f32 y = NORMALIZE(xpad->sThumbLY);
        if (abs_t(f32, x) <= threshold/2.0f) x = 0.0f;
        if (abs_t(f32, y) <= threshold/2.0f) y = 0.0f;
        pad->left_stick.x = x;
        pad->left_stick.y = y;
      }

      {
        f32 threshold = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE/32767.0f;
        f32 x = NORMALIZE(xpad->sThumbRX);
        f32 y = NORMALIZE(xpad->sThumbRY);
        if (abs_t(f32, x) <= threshold) x = 0.0f;
        if (abs_t(f32, y) <= threshold) y = 0.0f;
        pad->right_stick.x = x;
        pad->right_stick.y = y;
      }
      #undef NORMALIZE
    } else {
      break;
    }
  }
}

internal void
entry_point(int argc, char **argv) {
  platform_gamepad_init();

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

    u32 scale = 5;
    u32 render_w = 128;
    u32 render_h = 128;
    u32 window_w = render_w*scale;
    u32 window_h = render_h*scale;

    String8 window_title = str8_lit("STARFIGHTER");
    Platform_Handle window = platform_window_open(window_title, window_w, window_h);

    Image back_buffer = image_alloc(render_w, render_h);
    Input input = {0};

    Platform_Graphics_Info graphics_info = platform_get_graphics_info();
    Clock time = { .dt_sec = 1.0f/graphics_info.refresh_rate };
    // Clock time = { .dt_sec = 1.0f/60.0f };
    // Clock time = { .dt_sec = 1.0f/30.0f };

    Thread_Context *thread_context = thread_context_selected();
    Memory memory = memory_alloc(GB(1));

    platform_window_toggle_fullscreen(window);
    platform_window_show(window);

    u64 time_start = platform_get_time_us();

    for (b32 quit = false; !quit;) {
      Temp scratch = scratch_begin(0, 0);
      Platform_Event_List event_list = platform_get_event_list(scratch.arena);
      for (Platform_Event *event = event_list.first; event != 0; event = event->next) {
        switch (event->type) {
          case PLATFORM_EVENT_WINDOW_CLOSE: {
            quit = true;
            break;
          } break;
          case PLATFORM_EVENT_KEY_PRESS:
          case PLATFORM_EVENT_KEY_RELEASE: {
            Keycode keycode = event->keycode;
            b32 is_down = (event->type == PLATFORM_EVENT_KEY_PRESS);
            process_digital_button(keys + keycode, is_down);
          } break;
        }
      }
      if (quit) break;
      platform_gamepad_update();

#if BUILD_DEBUG
      if (keys[KEY_R].pressed) {
        libkrueger_unload(lib);
        lib = libkrueger_load(dst_lib_path, src_lib_path);
      }
#endif

#if BUILD_DEBUG
      input.keys      = keys;
#endif
      Gamepad *pad    = gamepads;
      input.confirm   = (pad->a.is_down)     ? pad->a     : keys[KEY_Z];
      input.pause     = (pad->back.is_down)  ? pad->back  : keys[KEY_P];
      input.quit      = (pad->start.is_down) ? pad->start : keys[KEY_Q];
      input.up        = (pad->up.is_down)    ? pad->up    : keys[KEY_UP];
      input.down      = (pad->down.is_down)  ? pad->down  : keys[KEY_DOWN];
      input.left      = (pad->left.is_down)  ? pad->left  : keys[KEY_LEFT];
      input.right     = (pad->right.is_down) ? pad->right : keys[KEY_RIGHT];
      input.shoot     = (pad->x.is_down) ? pad->x : keys[KEY_Z];
      input.bomb      = (pad->b.is_down) ? pad->b : keys[KEY_X];
      input.direction = pad->left_stick;

      if (keys[KEY_F11].pressed) platform_window_toggle_fullscreen(window);
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
     
      s32 width = back_buffer.width;
      s32 height = back_buffer.height;
      u32 *pixels = back_buffer.pixels;
      platform_window_blit(window, pixels, width, height);
      
      for (u32 key = 0; key < KEY_MAX; ++key) {
        keys[key].pressed = false;
        keys[key].released = false;
      }

      scratch_end(scratch);
    }

    platform_window_close(window);
    libkrueger_unload(lib);
  }
}
