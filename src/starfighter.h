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
  PLATFORM_API(platform_reserve, void *, (uxx size)) \
  PLATFORM_API(platform_commit, b32, (void *ptr, uxx size)) \
  PLATFORM_API(platform_decommit, void, (void *ptr, uxx size)) \
  PLATFORM_API(platform_release, void, (void *ptr, uxx size)) \
  PLATFORM_API(platform_read_entire_file, void *, (Arena *arena, String8 file_path)) \
  PLATFORM_API(platform_get_date_time, Date_Time, (void))

typedef struct {
  b32 is_initialized;
  String8 res_path;

#define PLATFORM_API(name, r, p) r (*name) p;
  PLATFORM_API_LIST
#undef PLATFORM_API

  uxx size;
  u8 *ptr; // NOTE: REQUIRED to be cleared at startup
} Memory;

typedef struct {
  u32 sample_rate;
  u32 num_channels;
  u32 num_frames;
  s16 *frames;
} Sound_Buffer;

#define GAME_PROC_LIST \
  GAME_PROC(frame) \
  GAME_PROC(output_sound)

#define GAME_FRAME_PROC(x) b32 x(Thread_Context *tctx, Memory *memory, Image *image, Input *input, Clock *time)
#define GAME_OUTPUT_SOUND_PROC(x) void x(Sound_Buffer *sound_buffer, Memory *memory)

typedef GAME_FRAME_PROC(game_frame_proc);
typedef GAME_OUTPUT_SOUND_PROC(game_output_sound_proc);

#endif // KRUEGER_SHARED_H
