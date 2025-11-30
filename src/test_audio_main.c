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

enum {
  WAVE_CHUNK_RIFF = (((u32)('R')<<0)|((u32)('I')<<8)|((u32)('F')<<16)|((u32)('F')<<24)),
  WAVE_CHUNK_WAVE = (((u32)('W')<<0)|((u32)('A')<<8)|((u32)('V')<<16)|((u32)('E')<<24)),
  WAVE_CHUNK_FMT  = (((u32)('f')<<0)|((u32)('m')<<8)|((u32)('t')<<16)|((u32)(' ')<<24)),
  WAVE_CHUNK_DATA = (((u32)('d')<<0)|((u32)('a')<<8)|((u32)('t')<<16)|((u32)('a')<<24)),
};

#pragma pack(push, 1)
typedef struct {
  u32 riff_id;
  u32 size;
  u32 wave_id;
} Wave_Header;

typedef struct {
  u32 id;
  u32 size;
} Wave_Chunk;

typedef struct {
  u16 audio_format;
  u16 num_channels;
  u32 sample_rate;
  u32 byte_rate;
  u16 block_align;
  u16 bits_per_sample;
} Wave_Format;
#pragma pack(pop)

typedef struct {
  u8 *at;
  u8 *stop;
} Riff_Iterator;

internal Riff_Iterator
riff_parse_chunk_at(void *at, void *stop) {
  Riff_Iterator result = {
    .at = (u8 *)at,
    .stop = (u8 *)stop,
  };
  return(result);
}

internal Riff_Iterator
riff_next_chunk(Riff_Iterator iter) {
  Wave_Chunk *chunk = (Wave_Chunk *)iter.at;
  u32 size = (chunk->size + 1) & ~1;
  iter.at += sizeof(Wave_Chunk) + size;
  return(iter);
}

internal b32
riff_is_valid(Riff_Iterator iter) {
  b32 result = (iter.at < iter.stop);
  return(result);
}

internal u32
riff_get_chunk_type(Riff_Iterator iter) {
  Wave_Chunk *chunk = (Wave_Chunk *)iter.at;
  u32 result = chunk->id;
  return(result);
}

internal u32
riff_get_chunk_data_size(Riff_Iterator iter) {
  Wave_Chunk *chunk = (Wave_Chunk *)iter.at;
  u32 result = chunk->size;
  return(result);
}

internal void *
riff_get_chunk_data(Riff_Iterator iter) {
  void *result = iter.at + sizeof(Wave_Chunk);
  return(result);
}

typedef struct {
  u32 size;
  void *data;
} Audio;

internal Audio
load_wav(Arena *arena, String8 file_path) {
  Audio result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  void *file_data = platform_read_entire_file(scratch.arena, file_path);
  if (file_data) {
    Wave_Header *header = (Wave_Header *)file_data;
    assert(header->riff_id == WAVE_CHUNK_RIFF);
    assert(header->wave_id == WAVE_CHUNK_WAVE);

    u8 *at = (u8 *)(header + 1);
    u8 *stop = (u8 *)(at + header->size - 4);
  
    u32 audio_size = 0;
    void *audio_data = 0;

    for (Riff_Iterator iter = riff_parse_chunk_at(at, stop);
         riff_is_valid(iter);
         iter = riff_next_chunk(iter)) {
      switch (riff_get_chunk_type(iter)) {
        case WAVE_CHUNK_FMT: {
          Wave_Format *format = riff_get_chunk_data(iter);
          assert(format->audio_format == 1); // NOTE: PCM
          assert(format->num_channels == 2);
          assert(format->sample_rate == 48000);
          assert(format->block_align == (format->num_channels*2));
          assert(format->bits_per_sample == 16);
        } break;
        case WAVE_CHUNK_DATA: {
          audio_size = riff_get_chunk_data_size(iter);
          audio_data = riff_get_chunk_data(iter);
        } break;
      }
    }

    assert(audio_size);
    assert(audio_data);
    
    result.size = audio_size;
    result.data = arena_push(arena, result.size);
    mem_copy(result.data, audio_data, audio_size);
  } else {
    log_error("%s: failed to read file: %s", __func__, file_path);
  }
  scratch_end(scratch);
  return(result);
}

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
  wave_format.nBlockAlign = (WORD)bytes_per_sample;
  wave_format.nAvgBytesPerSec = samples_per_second*bytes_per_sample;

  IXAudio2SourceVoice *source_voice;
  IXAudio2_CreateSourceVoice(audio_handle, &source_voice, &wave_format,
                             0, XAUDIO2_DEFAULT_FREQ_RATIO, 0, 0, 0);

#if 0
  // NOTE: Output Sine Wave
  f32 volume = 0.2f;
  u32 cycles_per_second = 256;
  u32 samples_per_cycle = samples_per_second/cycles_per_second;

  u32 audio_buffer_size_in_cycles = 10;
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
#else
  // NOTE: Output Wav File Audio
  Arena *arena = arena_alloc();
  Audio audio = load_wav(arena, str8_lit("../res/fireflies.wav"));

  IXAudio2SourceVoice_SubmitSourceBuffer(source_voice, (&(XAUDIO2_BUFFER){
    .Flags = XAUDIO2_END_OF_STREAM,
    .AudioBytes = audio.size,
    .pAudioData = (u8 *)audio.data,
    .PlayBegin = 0,
    .PlayLength = 0,
    .LoopBegin = 0,
    .LoopLength = 0,
    .LoopCount = XAUDIO2_LOOP_INFINITE,
  }), 0);
  IXAudio2SourceVoice_Start(source_voice, 0, XAUDIO2_COMMIT_NOW);
#endif

  u64 time_start = platform_get_time_us();
  u64 cycles_start = __rdtsc();

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

    if (quit) break;

    u64 cycles_end = __rdtsc();
    u64 time_end = platform_get_time_us();

    u64 dt_cycles = cycles_end - cycles_start;
    u64 dt_us = time_end - time_start;

    time_start = time_end;
    cycles_start = cycles_end;

    f32 mc = dt_cycles/million(1.0f);
    f32 ms = dt_us/thousand(1.0f);
    f32 fps = million(1.0f)/dt_us;
    log_info("%.2ffps, %.2fms, %.2fmc", fps, ms, mc);

    scratch_end(scratch);
  }

  platform_window_close(window);
}
