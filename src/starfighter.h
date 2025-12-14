#ifndef STARFIGHTER_H
#define STARFIGHTER_H

typedef struct {
  b32 is_down;
  b32 pressed;
  b32 released;
} Digital_Button;

typedef struct {
  f32 value;
  b32 is_down;
  b32 pressed;
  b32 released;
} Analog_Button;

typedef union {
  struct { f32 x, y; };
  Vector2 axis;
} Stick;

typedef struct {
#if BUILD_DEBUG
  Digital_Button *keys;
#endif

  Digital_Button confirm;
  Digital_Button pause;
  Digital_Button quit;

  Digital_Button up;
  Digital_Button down;
  Digital_Button left;
  Digital_Button right;

  Digital_Button shoot;
  Digital_Button bomb;

  Stick direction;
} Input;

typedef struct {
  // NOTE: this is the one that is supposed
  // to be used to calculate physics.
  f32 dt_sec;

  // NOTE: this is the real time per frame
  f32 _dt_ms;
  u64 _dt_us;
} Clock;

#define PLATFORM_API_LIST \
  PLATFORM_API(platform_reserve, void *, uxx) \
  PLATFORM_API(platform_commit, b32, void *, uxx) \
  PLATFORM_API(platform_decommit, void, void *, uxx) \
  PLATFORM_API(platform_release, void, void *, uxx) \
  PLATFORM_API(platform_read_entire_file, void *, Arena *, String8) \
  PLATFORM_API(platform_get_date_time, Date_Time, void)

typedef struct {
  b32 is_initialized;

#define PLATFORM_API(name, ret, ...) ret (*name)(__VA_ARGS__);
  PLATFORM_API_LIST
#undef PLATFORM_API

  String8 res_path;
  uxx size;
  u8 *ptr; // NOTE: REQUIRED to be cleared at startup
} Memory;

#define GAME_FRAME_PROC(x) b32 x(Thread_Context *thread_context, Memory *memory, Image *back_buffer, Input *input, Clock *time)
#define GAME_OUTPUT_SOUND_PROC(x) void x(s16 *buffer, u32 num_frames, u32 sample_rate, u32 num_channels, void *user_data)

typedef GAME_FRAME_PROC(frame_proc);
typedef GAME_OUTPUT_SOUND_PROC(output_sound_proc);

#define GAME_PROC_LIST \
  GAME_PROC(frame) \
  GAME_PROC(output_sound)

#endif // KRUEGER_SHARED_H
