#ifndef KRUEGER_PLATFORM_AUDIO_H
#define KRUEGER_PLATFORM_AUDIO_H

//////////////
// NOTE: Types

#define PLATFORM_AUDIO_BITS_PER_SAMPLE 16

#define PLATFORM_AUDIO_CALLBACK(x) void x(s16 *frames, u32 num_frames, void *user_data)
typedef PLATFORM_AUDIO_CALLBACK(Platform_Audio_Callback);

typedef struct {
  u32 sample_rate;
  u32 num_channels;
  Platform_Audio_Callback *callback;
  void *user_data;
} Platform_Audio_Desc;

////////////////
// NOTE: Globals

global const u32 _audio_default_sample_rate = 44100;
global const u16 _audio_default_num_channels = 1;
global Platform_Audio_Desc _platform_audio_desc;

//////////////////////////////////
// NOTE: Helpers, Implemented Once

internal void _platform_audio_setup(Platform_Audio_Desc *desc);

/////////////////////////////////
// NOTE: Implemented Per-Platform

internal void platform_audio_init(Platform_Audio_Desc *desc);
internal void platform_audio_shutdown(void);

#endif // KRUEGER_PLATFORM_AUDIO_H
