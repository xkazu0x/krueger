#define BUILD_CONSOLE_INTERFACE 1

#define PLATFORM_FEATURE_GRAPHICS 1
#define PLATFORM_FEATURE_AUDIO 0

#define OGL_MAJOR_VER 3
#define OGL_MINOR_VER 1

#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_opengl.h"
#include "krueger_image.h"
#include "starfighter.h"

#include "krueger_base.c"
#include "krueger_platform.c"
#include "krueger_opengl.c"
#include "krueger_image.c"

#if PLATFORM_WINDOWS
#include <xinput.h>

#define XINPUT_PROC_LIST \
  XINPUT_PROC(xinput_get_state, DWORD, (DWORD dwUserIndex, XINPUT_STATE *pState)) \
  XINPUT_PROC(xinput_set_state, DWORD, (DWORD dwUserIndex, XINPUT_VIBRATION *pVibration))

#define XINPUT_PROC(name, r, p) typedef r name##_proc p;
XINPUT_PROC_LIST
#undef XINPUT_PROC

#define XINPUT_PROC(name, r, p) internal r name##_stub p {return(ERROR_DEVICE_NOT_CONNECTED);};
XINPUT_PROC_LIST
#undef XINPUT_PROC

#define XINPUT_PROC(name, r, p) global name##_proc *name = name##_stub;
XINPUT_PROC_LIST
#undef XINPUT_PROC
#endif

/////////////////////
// NOTE: Game Library

typedef struct {
  Platform_Handle h;
#define GAME_PROC(name) game_##name##_proc *name;
  GAME_PROC_LIST
#undef GAME_PROC
} Game_Library;

internal Game_Library
game_library_open(String8 dst_path, String8 src_path) {
  Game_Library lib = {0};
  if (platform_copy_file_path(dst_path, src_path)) {
    lib.h = platform_library_open(dst_path);
    if (platform_handle_is_valid(lib.h)) {
#define GAME_PROC(name) \
      lib.name = (game_##name##_proc *)platform_library_load_proc(lib.h, str8_lit(#name));
      GAME_PROC_LIST
#undef GAME_PROC
    } else {
      log_error("%s: failed to open library: [%s]", __func__, dst_path.str);
    }
  } else {
    log_error("%s: failed to copy file path: from [%s] to [%s]", __func__, src_path.str, dst_path.str);
  }
  return(lib);
}

internal void
game_library_close(Game_Library lib) {
  if (platform_handle_is_valid(lib.h)) {
    platform_library_close(lib.h);
  }
}

//////////////
// NOTE: Input

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
process_digital_button(Digital_Button *button, b32 is_down) {
  b32 was_down = button->is_down;
  button->pressed = !was_down && is_down; 
  button->released = was_down && !is_down;
  button->is_down = is_down;
}

internal void
process_analog_button(Analog_Button *button, f32 threshold, f32 value) {
  b32 was_down = button->is_down;
  button->is_down = (value >= threshold);
  button->pressed = !was_down && button->is_down;
  button->released = was_down && !button->is_down;
}

internal void
process_stick(Stick *stick, f32 threshold, f32 x, f32 y) {
  if (abs_t(f32, x) <= threshold) x = 0.0f;
  if (abs_t(f32, y) <= threshold) y = 0.0f;
  stick->x = x;
  stick->y = y;
}

#if PLATFORM_WINDOWS
internal void
platform_gamepad_init(void) {
  String8 libs[3];
  libs[0] = str8_lit("xinput1_4.dll");
  libs[1] = str8_lit("xinput1_3.dll");
  libs[2] = str8_lit("xinput9_1_0.dll");
  for (each_item(u32, lib_index, libs)) {
    String8 lib_name = libs[lib_index];
    Platform_Handle h = platform_library_open(lib_name);
    if (platform_handle_is_valid(h)) {
      xinput_get_state = (xinput_get_state_proc *)
        platform_library_load_proc(h, str8_lit("XInputGetState"));
      xinput_set_state = (xinput_set_state_proc *)
        platform_library_load_proc(h, str8_lit("XInputSetState"));
      break;
    }
  }
}

internal void
platform_gamepad_update(void) {
  u32 gamepad_count = min(GAMEPAD_MAX, XUSER_MAX_COUNT);
  for (each_node(u32, gamepad_index, gamepad_count)) {
    XINPUT_STATE xinput_state;;;
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
        process_analog_button(&pad->left_trigger, threshold, xpad->bLeftTrigger/255.0f);
        process_analog_button(&pad->right_trigger, threshold, xpad->bRightTrigger/255.0f);
      }

#define NORMALIZE(x) (2.0f*((x + 32768)/65535.0f) - 1.0f)
      {
        f32 threshold = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/32767.0f;
        f32 x = NORMALIZE(xpad->sThumbLX);
        f32 y = NORMALIZE(xpad->sThumbLY);
        process_stick(&pad->left_stick, threshold, x, y);
      }
      {
        f32 threshold = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE/32767.0f;
        f32 x = NORMALIZE(xpad->sThumbRX);
        f32 y = NORMALIZE(xpad->sThumbRY);
        process_stick(&pad->left_stick, threshold, x, y);
      }
#undef NORMALIZE
  } else {
    break;
    }
  }
}
#else
internal void
platform_gamepad_init(void) {
}

internal void
platform_gamepad_update(void) {
}
#endif

internal void
input_update(Input *input) {
  platform_gamepad_update();
#if BUILD_DEBUG
  input->keys       = keys;
#endif
  Gamepad *pad     = gamepads;
  input->confirm   = (pad->a.is_down)     ? pad->a     : keys[KEY_Z];
  input->pause     = (pad->back.is_down)  ? pad->back  : keys[KEY_P];
  input->quit      = (pad->start.is_down) ? pad->start : keys[KEY_Q];
  input->up        = (pad->up.is_down)    ? pad->up    : keys[KEY_UP];
  input->down      = (pad->down.is_down)  ? pad->down  : keys[KEY_DOWN];
  input->left      = (pad->left.is_down)  ? pad->left  : keys[KEY_LEFT];
  input->right     = (pad->right.is_down) ? pad->right : keys[KEY_RIGHT];
  input->shoot     = (pad->x.is_down)     ? pad->x     : keys[KEY_Z];
  input->bomb      = (pad->b.is_down)     ? pad->b     : keys[KEY_X];
  input->direction = pad->left_stick;
}

internal void
input_reset(void) {
  for (u32 key = 0; key < KEY_MAX; ++key) {
    keys[key].pressed = false;
    keys[key].released = false;
  }
}

//////////////
// NOTE: Audio

// typedef struct {
//   Memory *memory;
//   game_output_sound_proc *game_output_sound;
// } Audio_Callback_Data;

// internal
// PLATFORM_AUDIO_CALLBACK(audio_cb) {
//   Audio_Callback_Data *data = (Audio_Callback_Data *)user_data;
//   Sound_Buffer sound_buffer = {
//     .sample_rate = _platform_audio_desc.sample_rate,
//     .num_channels = _platform_audio_desc.num_channels,
//     .num_frames = num_frames,
//     .frames = frames,
//   };
//   if (data->game_output_sound) {
//     data->game_output_sound(&sound_buffer, data->memory);
//   } else {
//     mem_zero_typed(frames, num_frames*sound_buffer.num_channels);
//   }
// }

//////////////
// NOTE: Misc.

internal Memory
memory_alloc(uxx size) {
  Memory result = {0};
  result.size = size;
  result.ptr = platform_reserve(size);
  platform_commit(result.ptr, size);
  return(result);
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

internal String8
path_find_sub_directory(Arena *arena, String8 path, String8 sub) {
  Temp scratch = scratch_begin(&arena, 1);
  String8_List path_list = str8_split_path(scratch.arena, path);
  String8_List sub_list = {0};
  for (each_node(String8_Node, node, path_list.first)) {
    str8_list_push(scratch.arena, &sub_list, node->string);
    if (str8_match(node->string, sub)) {
      break;
    }
  }
#if PLATFORM_WINDOWS
  String8 result = str8_list_join(arena, &sub_list, &(String_Join){
    .sep = str8_lit("\\"),
    .post = str8_lit("\\"),
  });
#elif PLATFORM_LINUX
  String8 result = str8_list_join(arena, &sub_list, &(String_Join){
    .sep = str8_lit("/"),
    .post = str8_lit("/"),
  });
#endif
    return(result);
}

internal void
render_image_to_window(Platform_Handle window, Image image) {
  Rect2 client_rect = platform_get_window_client_rect(window);
  Vector2 window_size = vector2_sub(client_rect.max, client_rect.min);
  glViewport(0, 0, (s32)window_size.x, (s32)window_size.y);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glBindTexture(GL_TEXTURE_2D, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, image.pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glEnable(GL_TEXTURE_2D);

  f32 display_w = image.width*(window_size.y/image.height);
  f32 display_h = image.height*(window_size.x/image.width);

  if (window_size.x >= display_w) {
    display_h = window_size.y;
  } else if (window_size.y >= display_h) {
    display_w = window_size.x;
  }

  f32 x = display_w/window_size.x;
  f32 y = display_h/window_size.y;

  glBegin(GL_TRIANGLES); {
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-x, y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x, y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x, -y);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-x, y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x, -y);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-x, -y);
  } glEnd();

  ogl_window_swap(window);
}

internal void
entry_point(int argc, char **argv) {
  platform_gamepad_init();

  Arena *arena = arena_alloc();
  String8 exec_file_path = platform_get_exec_file_path(arena);
  String8 root_path = path_find_sub_directory(arena, exec_file_path, str8_lit("krueger"));

#if PLATFORM_WINDOWS
  String8 build_path   = str8_cat(arena, root_path, str8_lit("build\\"));
  String8 res_path     = str8_cat(arena, root_path, str8_lit("res\\"));
  String8 src_lib_name = str8_lit("libstarfighter.dll");
  String8 dst_lib_name = str8_lit("libstarfighterx.dll");
#elif PLATFORM_LINUX
  String8 build_path   = str8_cat(arena, root_path, str8_lit("build/"));
  String8 res_path     = str8_cat(arena, root_path, str8_lit("res/"));
  String8 src_lib_name = str8_lit("libstarfighter.so");
  String8 dst_lib_name = str8_lit("libstarfighterx.so");
#endif

  String8 src_lib_path = str8_cat(arena, build_path, src_lib_name);
  String8 dst_lib_path = str8_cat(arena, build_path, dst_lib_name);

  Game_Library game = game_library_open(dst_lib_path, src_lib_path);
  if (platform_handle_is_valid(game.h)) {
    u32 window_scale = 5;
    u32 render_w = 128;
    u32 render_h = 128;

    String8 window_name = str8_lit("STARFIGHTER");
    u32 window_w = window_scale*render_w;
    u32 window_h = window_scale*render_h;

    Platform_Handle window = platform_window_open(window_name, window_w, window_h);
    ogl_window_equip(window);
    ogl_window_select(window);

    Image back_buffer = image_alloc(render_w, render_h);

    Platform_Graphics_Info graphics_info = platform_get_graphics_info();
    Clock time = {.dt_sec = 1.0f/graphics_info.refresh_rate};
    Input input = {0};

    Thread_Context *tctx = thread_context_selected();
    Memory memory = memory_alloc(GB(1));
    memory.res_path = res_path;
#define PLATFORM_API(name, ret, ...) memory.name = name;
    PLATFORM_API_LIST
#undef PLATFORM_API

    // Audio_Callback_Data audio_cb_data = {
    //   .memory = &memory,
    //   .game_output_sound = game.output_sound,
    // };

    // platform_audio_init(&(Platform_Audio_Desc){
    //   .sample_rate = 48000,
    //   .num_channels = 2,
    //   .callback = audio_cb,
    //   .user_data = &audio_cb_data,
    // });

    platform_window_set_fullscreen(window, false);
    platform_window_show(window);

    u64 time_start = platform_get_time_us();

    for (b32 quit = false; !quit;) {
      Temp scratch = scratch_begin(0, 0);
      Platform_Event_List event_list = platform_get_event_list(scratch.arena);
      for (each_node(Platform_Event, event, event_list.first)) {
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
      scratch_end(scratch);
      if (quit) break;

#if BUILD_DEBUG
      if (keys[KEY_R].pressed) {
        // audio_cb_data.game_output_sound = 0;
        game_library_close(game);
        game = game_library_open(dst_lib_path, src_lib_path);
        // audio_cb_data.game_output_sound = game.output_sound;
      }
#endif

      if (keys[KEY_F11].pressed) {
        b32 fullscreen = platform_window_is_fullscreen(window);
        platform_window_set_fullscreen(window, !fullscreen);
      }

      input_update(&input);
      if (game.frame) {
        quit = game.frame(tctx, &memory, &back_buffer, &input, &time);
        if (quit) break;
      }

      wait_to_flip(time.dt_sec, time_start);
      u64 time_end = platform_get_time_us();
      time._dt_us = time_end - time_start;
      time._dt_ms = time._dt_us/thousand(1.0f);
      time_start = time_end;

      render_image_to_window(window, back_buffer);
      input_reset();
    }
  }
}
