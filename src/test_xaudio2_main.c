#define BUILD_CONSOLE_INTERFACE 1
#define PLATFORM_FEATURE_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#include <combaseapi.h>
#include <xaudio2.h>

#pragma comment(lib, "xaudio2")
#pragma comment(lib, "ole32")

internal void
entry_point(int argc, char **argv) {
  Platform_Handle window = platform_window_open(str8_lit("krueger"), 800, 600);
  platform_window_show(window);

  CoInitializeEx(0, COINIT_MULTITHREADED);

  IXAudio2 *audio_handle;
  XAudio2Create(&audio_handle, 0, XAUDIO2_DEFAULT_PROCESSOR);

  IXAudio2MasteringVoice *mastering_voice;
  IXAudio2_CreateMasteringVoice(audio_handle, &mastering_voice,
                                2, 48000, 0, 0, 0, 0);

  u32 num_channels = 2;
  u32 bits_per_channel = 16;
  u32 bytes_per_sample = (num_channels*bits_per_channel)/8;
  u32 samples_per_second = 48000;

  WAVEFORMATEX wave_format = {0};
  wave_format.wFormatTag = WAVE_FORMAT_PCM;
  wave_format.nChannels = (WORD)num_channels;
  wave_format.wBitsPerSample = (WORD)bits_per_channel;
  wave_format.nSamplesPerSec = samples_per_second;
  wave_format.nBlockAlign = (wave_format.nChannels*wave_format.wBitsPerSample)/8;
  wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec*wave_format.nBlockAlign;

  IXAudio2SourceVoice *source_voice;
  IXAudio2_CreateSourceVoice(audio_handle, &source_voice, &wave_format,
                             0, XAUDIO2_DEFAULT_FREQ_RATIO, 0, 0, 0);

  f32 volume = 0.5f;
  u32 cycles_per_second = 256;
  u32 samples_per_cycle = samples_per_second/cycles_per_second;

  u32 audio_buffer_size_in_cycles = 8;
  u32 audio_buffer_size_in_samples = samples_per_cycle*audio_buffer_size_in_cycles;
  u32 audio_buffer_size_in_bytes = bytes_per_sample*audio_buffer_size_in_samples;

  Arena *arena = arena_alloc();
  u8 *samples = arena_push(arena, audio_buffer_size_in_bytes);

  f32 phase = 0.0f;
  s16 *dst = (s16 *)samples;
  for (u32 sample_index = 0;
       sample_index < audio_buffer_size_in_samples;
       ++sample_index) {
    phase += tau32/(f32)samples_per_cycle;
    f32 sine_value = sin_f32(phase);
    s16 sample_value = (s16)(sine_value*s16_max*volume);
    *dst++ = sample_value;
    *dst++ = sample_value;
  }

  IXAudio2SourceVoice_SubmitSourceBuffer(source_voice, (&(XAUDIO2_BUFFER){
    .Flags = XAUDIO2_END_OF_STREAM,
    .AudioBytes = audio_buffer_size_in_bytes,
    .pAudioData = samples,
    .PlayBegin = 0,
    .PlayLength = 0,
    .LoopBegin = 0,
    .LoopLength = 0,
    .LoopCount = XAUDIO2_LOOP_INFINITE,
  }), 0);
  IXAudio2SourceVoice_Start(source_voice, 0, XAUDIO2_COMMIT_NOW);

  for (b32 quit = false; !quit;) {
    Temp scratch = scratch_begin(0, 0);
    Platform_Event_List event_list = platform_get_event_list(scratch.arena);
    for (Platform_Event *event = event_list.first; event != 0; event = event->next) {
      switch (event->type) {
        case PLATFORM_EVENT_WINDOW_CLOSE: {
          quit = true;
        } break;
      }
    }
    scratch_end(scratch);
  }

  platform_window_close(window);
}
