#define BUILD_CONSOLE_INTERFACE 1
#define PLATFORM_FEATURE_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#define replace_null(val, is_null) (((val) == 0) ? (is_null) : (val))

typedef struct {
  u32 sample_rate;
  u32 num_channels;
} Audio_Format;

typedef struct {
  Audio_Format format;
  u32 size;
  void *data;
} Audio;

#define encode_u32(a, b, c, d) (((u32)(a)<<0)|((u32)(b)<<8)|((u32)(c)<<16)|((u32)(d)<<24))

enum {
  WAVE_CHUNK_RIFF = encode_u32('R', 'I', 'F', 'F'),
  WAVE_CHUNK_WAVE = encode_u32('W', 'A', 'V', 'E'),
  WAVE_CHUNK_FMT  = encode_u32('f', 'm', 't', ' '),
  WAVE_CHUNK_DATA = encode_u32('d', 'a', 't', 'a'),
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
    
    Audio_Format format = {0};
    u32 audio_size = 0;
    void *audio_data = 0;

    for (Riff_Iterator iter = riff_parse_chunk_at(at, stop);
         riff_is_valid(iter);
         iter = riff_next_chunk(iter)) {
      switch (riff_get_chunk_type(iter)) {
        case WAVE_CHUNK_FMT: {
          Wave_Format *audio_format = riff_get_chunk_data(iter);
          assert(audio_format->audio_format == 1); // NOTE: PCM
          assert(audio_format->num_channels == 2);
          assert(audio_format->sample_rate == 48000);
          assert(audio_format->block_align == (audio_format->num_channels*2));
          assert(audio_format->bits_per_sample == 16);
          format.num_channels = audio_format->num_channels;
          format.sample_rate = audio_format->sample_rate;
        } break;
        case WAVE_CHUNK_DATA: {
          audio_size = riff_get_chunk_data_size(iter);
          audio_data = riff_get_chunk_data(iter);
        } break;
      }
    }

    assert(audio_size);
    assert(audio_data);
    
    result.format = format;
    result.size = audio_size;
    result.data = arena_push(arena, result.size);
    mem_copy(result.data, audio_data, audio_size);
  } else {
    log_error("%s: failed to read file: %s", __func__, file_path);
  }
  scratch_end(scratch);
  return(result);
}

#pragma comment(lib, "ole32")
#ifndef CINTERFACE
#define CINTERFACE
#endif
#ifndef COBJMACROS
#define COBJMACROS
#endif
#ifndef CONST_VTABLE
#define CONST_VTABLE
#endif
#include <mmdeviceapi.h>
#include <audioclient.h>
global const IID    _win32_IID_IMMDeviceEnumerator  = { 0xA95664D2, 0x9614, 0x4F35, {0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6} };
global const CLSID  _win32_CLSID_MMDeviceEnumerator = { 0xBCDE0395, 0xE52F, 0x467C, {0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E} };
global const IID    _win32_IID_IAudioClient         = { 0x1CB9AD4C, 0xDBFA, 0x4c32, {0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2} };
global const IID    _win32_IID_IAudioRenderClient   = { 0xf294acfc, 0x3146, 0x4483, {0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2} };
#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
#define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000
#endif
#ifndef AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
#define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000
#endif

#define WIN32_CHECK(x) assert((x) == S_OK)
#define BITS_PER_SAMPLE 16

#define PLATFORM_AUDIO_CALLBACK(x) void x(s16 *samples, u32 num_samples, void *user_data)
typedef PLATFORM_AUDIO_CALLBACK(Platform_Audio_Callback);

typedef struct Platform_Audio_Desc {
  u32 sample_rate;
  u32 num_channels;
  Platform_Audio_Callback *callback;
  void *user_data;
} Platform_Audio_Desc;

typedef struct Win32_Audio_Backend {
  HANDLE buffer_end_event;
  IMMDeviceEnumerator *device_enumerator;
  IMMDevice *device;
  IAudioClient *audio_client;
  IAudioRenderClient *render_client;
} Win32_Audio_Backend;

typedef struct Win32_Audio_Backend Platform_Audio_Backend;
typedef struct Platform_Audio_State {
  Platform_Audio_Callback *callback;
  void *user_data;
  u32 sample_rate;
  u32 num_channels;
  Platform_Audio_Backend backend;
} Platform_Audio_State;

global Platform_Audio_State _platform_audio_state;

global const u32 audio_default_sample_rate = 44100;
global const u16 audio_default_num_channels = 1;

internal DWORD WINAPI
_win32_wasapi_thread_proc(LPVOID param) {
  u32 buffer_frame_count;
  IAudioClient_GetBufferSize(_platform_audio_state.backend.audio_client, &buffer_frame_count);

  IAudioClient_Start(_platform_audio_state.backend.audio_client);
  for (;;) {
    WaitForSingleObject(_platform_audio_state.backend.buffer_end_event, INFINITE);
    u32 padding_frame_count;
    IAudioClient_GetCurrentPadding(_platform_audio_state.backend.audio_client, &padding_frame_count);
    u32 fill_frame_count = buffer_frame_count - padding_frame_count;

    s16 *samples = 0;
    IAudioRenderClient_GetBuffer(_platform_audio_state.backend.render_client, fill_frame_count, (BYTE **)&samples);
    u32 num_samples = fill_frame_count*_platform_audio_state.num_channels;

    if (_platform_audio_state.callback) {
      _platform_audio_state.callback(samples, num_samples, _platform_audio_state.user_data);
    }

    IAudioRenderClient_ReleaseBuffer(_platform_audio_state.backend.render_client, fill_frame_count, 0);
  }
  return(0);
}

internal void
_win32_wasapi_backend_init(void) {
  WIN32_CHECK(CoInitializeEx(0, COINIT_MULTITHREADED));
  _platform_audio_state.backend.buffer_end_event = CreateEvent(0, 0, 0, 0);

  WIN32_CHECK(CoCreateInstance(&_win32_CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, &_win32_IID_IMMDeviceEnumerator, &_platform_audio_state.backend.device_enumerator));
  WIN32_CHECK(IMMDeviceEnumerator_GetDefaultAudioEndpoint(_platform_audio_state.backend.device_enumerator, eRender, eConsole, &_platform_audio_state.backend.device));
  WIN32_CHECK(IMMDevice_Activate(_platform_audio_state.backend.device, &_win32_IID_IAudioClient, CLSCTX_ALL, 0, &_platform_audio_state.backend.audio_client));

  WAVEFORMATEX audio_format = {0};
  audio_format.wFormatTag = WAVE_FORMAT_PCM;
  audio_format.nChannels = (WORD)_platform_audio_state.num_channels;
  audio_format.nSamplesPerSec = _platform_audio_state.sample_rate;
  audio_format.wBitsPerSample = BITS_PER_SAMPLE;
  audio_format.nBlockAlign = (audio_format.nChannels*audio_format.wBitsPerSample)/8;
  audio_format.nAvgBytesPerSec = audio_format.nSamplesPerSec*audio_format.nBlockAlign;

  u32 audio_client_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
  WIN32_CHECK(IAudioClient_Initialize(_platform_audio_state.backend.audio_client, AUDCLNT_SHAREMODE_SHARED, audio_client_flags, 0, 0, &audio_format, 0));
  WIN32_CHECK(IAudioClient_GetService(_platform_audio_state.backend.audio_client, &_win32_IID_IAudioRenderClient, &_platform_audio_state.backend.render_client));
  WIN32_CHECK(IAudioClient_SetEventHandle(_platform_audio_state.backend.audio_client, _platform_audio_state.backend.buffer_end_event));

  CreateThread(0, 0, _win32_wasapi_thread_proc, 0, 0, 0);
}

internal void
_win32_wasapi_backend_shutdown(void) {
  IAudioRenderClient_Release(_platform_audio_state.backend.render_client);
  IAudioClient_Release(_platform_audio_state.backend.audio_client);
  IMMDevice_Release(_platform_audio_state.backend.device);
  IMMDeviceEnumerator_Release(_platform_audio_state.backend.device_enumerator);
  CoUninitialize();
}

internal void
platform_audio_init(Platform_Audio_Desc *desc) {
  _platform_audio_state.callback = desc->callback;
  _platform_audio_state.user_data = desc->user_data;
  _platform_audio_state.sample_rate = replace_null(desc->sample_rate, audio_default_sample_rate);
  _platform_audio_state.num_channels = replace_null(desc->num_channels, audio_default_num_channels);
  _win32_wasapi_backend_init();
}

internal void
platform_audio_shutdown(void) {
  _win32_wasapi_backend_shutdown();
}

internal u32
platform_audio_sample_rate(void) {
  return(_platform_audio_state.sample_rate);
}

internal u32
platform_audio_channels(void) {
  return(_platform_audio_state.num_channels);
}

typedef struct {
  Arena *arena;

  f32 t_sine;
  f32 tone_volume;
  f32 wave_period;

  Audio audio;
  u32 audio_index;
} Program_State;

internal
PLATFORM_AUDIO_CALLBACK(output_sound) {
#if 0
  u32 sample_rate = platform_audio_sample_rate();
  Program_State *state = (Program_State *)user_data;
  state->tone_volume = 4000.0f;
  state->wave_period = sample_rate/256.0f;
  for (u32 sample_index = 0;
       sample_index < num_samples;
       sample_index += 2) {
    f32 sine_value = sin_f32(state->t_sine);
    s16 sample_value = (s16)(sine_value*state->tone_volume);
    samples[sample_index + 0] = sample_value;
    samples[sample_index + 1] = sample_value;
    state->t_sine += tau32/state->wave_period;
    if (state->t_sine > tau32) state->t_sine -= tau32;
  }
#else
  Program_State *state = (Program_State *)user_data;
  u32 src_sample_count = state->audio.size/2;
  s16 *src = (s16 *)state->audio.data;
  for (u32 sample_index = 0;
       sample_index < num_samples;
       sample_index += 2) {
    samples[sample_index + 0] = src[state->audio_index++ % src_sample_count];
    samples[sample_index + 1] = src[state->audio_index++ % src_sample_count];
  }
#endif
}

internal void
entry_point(int argc, char **argv) {
  Platform_Handle window = platform_window_open(str8_lit("krueger"), 800, 600);
  platform_window_show(window);

  Arena *arena = arena_alloc();
  Program_State *state = push_array(arena, Program_State, 1);
  state->arena = arena;
  state->audio = load_wav(arena, str8_lit("../res/fireflies.wav"));

  platform_audio_init(&(Platform_Audio_Desc){
    .sample_rate = 48000,
    .num_channels = 2,
    .callback = output_sound,
    .user_data = state,
  });

  for (b32 quit = false; !quit;) {
    Temp scratch = scratch_begin(0, 0);
    Platform_Event_List event_list = platform_get_event_list(scratch.arena);
    for (Platform_Event *event = event_list.first; event != 0; event = event->next) {
      switch (event->type) {
        case PLATFORM_EVENT_WINDOW_CLOSE: {
          quit = true;
          break;
        } break;
      }
    }
    scratch_end(scratch);
  }

  platform_audio_shutdown();
}
