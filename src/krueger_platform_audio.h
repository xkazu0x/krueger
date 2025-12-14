#ifndef KRUEGER_PLATFORM_AUDIO_H
#define KRUEGER_PLATFORM_AUDIO_H

//////////////
// NOTE: Types

#define BITS_PER_SAMPLE 16

typedef struct {
  u32 sample_rate;
  u32 num_channels;
  void (*callback)(s16 *samples, u32 num_samples, u32 sample_rate, void *user_data);
  void *user_data;
} Platform_Audio_Desc;

global Platform_Audio_Desc _audio_desc;
global const u32 audio_default_sample_rate = 44100;
global const u16 audio_default_num_channels = 1;

//////////////////////////////////
// NOTE: Helpers, Implemented Once

internal void _platform_setup_audio_desc(Platform_Audio_Desc *desc);

/////////////////////////////////
// NOTE: Implemented Per-Platform

internal void platform_audio_init(Platform_Audio_Desc *desc);
internal void platform_audio_shutdown(void);

#endif // KRUEGER_PLATFORM_AUDIO_H
