#define KRUEGER_PLATFORM_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#include <dsound.h>
// #pragma comment(lib, "dsound.lib")

#define DIRECT_SOUND_CREATE(x) HRESULT WINAPI x(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create_proc);

typedef struct {
  u16 bits_per_sample;
  u16 num_channels;
  u16 block_align;
  u32 sample_rate;
  u32 byte_rate;
} Audio_Format;

internal Audio_Format
make_audio_format(u32 sample_rate, u16 bits_per_sample, u16 num_channels) {
  Audio_Format result = {0};
  result.bits_per_sample = bits_per_sample;
  result.num_channels = num_channels;
  result.block_align = (bits_per_sample/8)*num_channels;
  result.sample_rate = sample_rate;
  result.byte_rate = sample_rate*result.block_align;
  return(result);
}

internal u32
buffer_size_from_audio_format(Audio_Format format) {
  u32 result = format.sample_rate*(format.bits_per_sample/8)*format.num_channels;
  return(result);
}

typedef struct {
  Audio_Format format;
  u32 size;
  s16 *data;
} Audio_File;

typedef struct Win32_Audio_Device Win32_Audio_Device;
struct Win32_Audio_Device {
  Win32_Audio_Device *next;
  Win32_Audio_Device *prev;
  Audio_Format format;
  LPDIRECTSOUND direct_sound;
  LPDIRECTSOUNDBUFFER primary_buffer;
  LPDIRECTSOUNDBUFFER secondary_buffer;
};

typedef struct {
  Arena *arena;
  Platform_Handle lib;
  Win32_Audio_Device *first_device;
  Win32_Audio_Device *last_device;
  Win32_Audio_Device *free_device;
} Win32_Audio_State;

global Win32_Audio_State *win32_audio_state;

internal Win32_Audio_Device *
win32_audio_device_alloc(void) {
  Win32_Audio_Device *result = win32_audio_state->free_device;
  if (result) {
    sll_stack_pop(win32_audio_state->free_device);
  } else {
    result = push_array(win32_audio_state->arena, Win32_Audio_Device, 1);
  }
  mem_zero_struct(result);
  dll_push_back(win32_audio_state->first_device,
                win32_audio_state->last_device,
                result);
  return(result);
}

internal void
win32_audio_device_release(Win32_Audio_Device *device) {
  IDirectSoundBuffer_Release(device->secondary_buffer);
  IDirectSoundBuffer_Release(device->primary_buffer);
  IDirectSound_Release(device->direct_sound);
  dll_remove(win32_audio_state->first_device,
             win32_audio_state->last_device,
             device);
  sll_stack_push(win32_audio_state->free_device, device);
}

internal void
platform_audio_init(void) {
  Arena *arena = arena_alloc();
  win32_audio_state = push_array(arena, Win32_Audio_State, 1);
  win32_audio_state->arena = arena;

  String8 lib_name = str8_lit("dsound.dll");
  win32_audio_state->lib = platform_library_open(lib_name);
  if (platform_handle_is_null(win32_audio_state->lib)) {
    log_error("%s: failed to open library: [%s]", __func__, lib_name.str);
  }
}

internal Platform_Handle
platform_audio_device_open(Platform_Handle window, Audio_Format format) {
  Platform_Handle result = {0};

  LPDIRECTSOUND direct_sound;
  LPDIRECTSOUNDBUFFER primary_buffer;
  LPDIRECTSOUNDBUFFER secondary_buffer;

  direct_sound_create_proc *direct_sound_create = (direct_sound_create_proc *)
    platform_library_load_proc(win32_audio_state->lib, "DirectSoundCreate");

  if (SUCCEEDED(direct_sound_create(0, &direct_sound, 0))) {
    WAVEFORMATEX wave_format = {0};
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = format.num_channels;
    wave_format.wBitsPerSample = format.bits_per_sample;
    wave_format.nBlockAlign = (wave_format.wBitsPerSample/8)*wave_format.nChannels;
    wave_format.nSamplesPerSec = format.sample_rate;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec*wave_format.nBlockAlign;

    Win32_Window *win32_window = win32_window_from_handle(window);
    if (SUCCEEDED(IDirectSound_SetCooperativeLevel(direct_sound,
                                                   win32_window->hwnd,
                                                   DSSCL_PRIORITY))) {
      DSBUFFERDESC primary_buffer_desc = {0};
      primary_buffer_desc.dwSize = sizeof(primary_buffer_desc);
      primary_buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;

      if (SUCCEEDED(IDirectSound_CreateSoundBuffer(direct_sound,
                                                   &primary_buffer_desc,
                                                   &primary_buffer,
                                                   0))) {
        HRESULT hr = IDirectSoundBuffer_SetFormat(primary_buffer, &wave_format);
        if (SUCCEEDED(hr)) {
          DSBUFFERDESC secondary_buffer_desc = {0};
          secondary_buffer_desc.dwSize = sizeof(secondary_buffer_desc);
          secondary_buffer_desc.dwFlags = 0;
          secondary_buffer_desc.dwBufferBytes = buffer_size_from_audio_format(format);
          secondary_buffer_desc.lpwfxFormat = &wave_format;

          hr = IDirectSound_CreateSoundBuffer(direct_sound,
                                              &secondary_buffer_desc,
                                              &secondary_buffer,
                                              0);
          if (SUCCEEDED(hr)) {
            Win32_Audio_Device *device = win32_audio_device_alloc();
            device->format = format;
            device->direct_sound = direct_sound;
            device->primary_buffer = primary_buffer;
            device->secondary_buffer = secondary_buffer;

            result.ptr[0] = cast(uxx)device;
          } else {
            log_error("%s: direct sound: failed to create secondary buffer", __func__);
          }
        } else {
          log_error("%s: direct sound: failed to set primary buffer format", __func__);
        }
      } else {
        log_error("%s: direct sound: failed to create primary buffer", __func__);
      }
    }
  }

  return(result);
}

global LPDIRECTSOUNDBUFFER secondary_buffer;

internal void
win32_direct_sound_init(Platform_Handle window, Audio_Format format) {
  Arena *arena = arena_alloc();
  win32_audio_state = push_array(arena, Win32_Audio_State, 1);
  win32_audio_state->arena = arena;

  String8 lib_name = str8_lit("dsound.dll");
  win32_audio_state->lib = platform_library_open(lib_name);
  if (!platform_handle_is_null(win32_audio_state->lib)) {
    direct_sound_create_proc *direct_sound_create = (direct_sound_create_proc *)
      platform_library_load_proc(win32_audio_state->lib, "DirectSoundCreate");

    LPDIRECTSOUND direct_sound;
    if (SUCCEEDED(direct_sound_create(0, &direct_sound, 0))) {
      WAVEFORMATEX wave_format = {0};
      wave_format.wFormatTag = WAVE_FORMAT_PCM;
      wave_format.wBitsPerSample = format.bits_per_sample;
      wave_format.nChannels = format.num_channels;
      wave_format.nBlockAlign = format.block_align;
      wave_format.nSamplesPerSec = format.sample_rate;
      wave_format.nAvgBytesPerSec = format.byte_rate;

      Win32_Window *win32_window = win32_window_from_handle(window);
      if (SUCCEEDED(IDirectSound_SetCooperativeLevel(direct_sound,
                                                     win32_window->hwnd,
                                                     DSSCL_PRIORITY))) {
        DSBUFFERDESC buffer_desc = {0};
        buffer_desc.dwSize = sizeof(buffer_desc);
        buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;

        LPDIRECTSOUNDBUFFER primary_buffer;
        if (SUCCEEDED(IDirectSound_CreateSoundBuffer(direct_sound,
                                                     &buffer_desc,
                                                     &primary_buffer,
                                                     0))) {
          HRESULT hr = IDirectSoundBuffer_SetFormat(primary_buffer, &wave_format);
          if (SUCCEEDED(hr)) {
            log_info("%s: direct sound: primary buffer format was set", __func__);
          } else {
            log_error("%s: direct sound: failed to set primary buffer format", __func__);
          }
        }
      }

      DSBUFFERDESC buffer_desc = {0};
      buffer_desc.dwSize = sizeof(buffer_desc);
      buffer_desc.dwFlags = 0;
      buffer_desc.dwBufferBytes = buffer_size_from_audio_format(format);
      buffer_desc.lpwfxFormat = &wave_format;

      HRESULT hr = IDirectSound_CreateSoundBuffer(direct_sound,
                                                  &buffer_desc,
                                                  &secondary_buffer,
                                                  0);
      if (SUCCEEDED(hr)) {
        log_info("%s: direct sound: secondary buffer created successfully", __func__);
      } else {
        log_error("%s: direct sound: failed to create secondary buffer", __func__);
      }
    }
  } else {
    log_error("%s: failed to open library: [%s]", __func__, lib_name.str);
  }
}

#pragma pack(push, 1)
typedef struct {
  u32 chunk_id;
  u32 chunk_size;
  u32 format;

  u32 subchunk1_id;
  u32 subchunk1_size;
  u16 audio_format;
  u16 num_channels;
  u32 sample_rate;
  u32 byte_rate;
  u16 block_align;
  u16 bits_per_sample;

  u32 subchunk2_id;
  u32 subchunk2_size;
  s16 *data;
} Wave_Header;
#pragma pack(pop)

internal void
load_wave(Arena *arena, String8 file_path) {
  Temp scratch = scratch_begin(&arena, 1);
  void *file_data = platform_read_entire_file(scratch.arena, file_path);
  if (file_data) {
    Wave_Header *header = (Wave_Header *)file_data;
    if (header->chunk_id == cstr_encode("RIFF")) {
      if (header->format == cstr_encode("WAVE")) {
        if (header->subchunk1_id == cstr_encode("fmt ")) {
          if (header->subchunk2_id == cstr_encode("data")) {
          }
        }
      }
    }
  } else {
    log_error("%s: failed to read file: %s", __func__, file_path);
  }
  scratch_end(scratch);
}

int
main(void) {
  platform_core_init();
  platform_graphics_init();

  Thread_Context *context = thread_context_alloc();
  thread_context_select(context);

  Arena *arena = arena_alloc();
  load_wave(arena, str8_lit("../res/test.wav"));

  Platform_Handle window = platform_window_open(str8_lit("sound test"), 800, 600);
  platform_window_show(window);

  Audio_Format audio_format = make_audio_format(48000, 16, 2);
  win32_direct_sound_init(window, audio_format);

  u32 running_sample_index = 0;
  u32 samples_per_sec = audio_format.sample_rate;
  u32 bytes_per_sample = audio_format.block_align;
  u32 latency_sample_count = audio_format.sample_rate/15;
  u32 secondary_buffer_size = buffer_size_from_audio_format(audio_format);

  s16 *samples = platform_reserve(secondary_buffer_size);
  platform_commit(samples, secondary_buffer_size);

  { // NOTE: Clear Sound Buffer
    VOID *region1;
    VOID *region2;
    DWORD region1_size;
    DWORD region2_size;
    if (SUCCEEDED(IDirectSoundBuffer_Lock(secondary_buffer,
                                          0, secondary_buffer_size,
                                          &region1, &region1_size,
                                          &region2, &region2_size,
                                          0))) {
      u8 *dst = (u8 *)region1;
      for (DWORD byte_index = 0;
           byte_index < region1_size;
           ++byte_index) {
        *dst++ = 0;
      }

      dst = (u8 *)region2;
      for (DWORD byte_index = 0;
           byte_index < region2_size;
           ++byte_index) {
        *dst++ = 0;
      }

      IDirectSoundBuffer_Unlock(secondary_buffer,
                                region1, region1_size,
                                region2, region2_size);
    }
  }

  IDirectSoundBuffer_Play(secondary_buffer, 0, 0, DSBPLAY_LOOPING);

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

    DWORD byte_to_lock = 0;
    DWORD bytes_to_write = 0;
    DWORD target_cursor;
    DWORD play_cursor;
    DWORD write_cursor;
    b32 sound_is_valid = false;
    if (SUCCEEDED(IDirectSoundBuffer_GetCurrentPosition(secondary_buffer,
                                                        &play_cursor,
                                                        &write_cursor))) {
      byte_to_lock = ((running_sample_index*bytes_per_sample) %
                      secondary_buffer_size);
      target_cursor = ((play_cursor + (latency_sample_count*bytes_per_sample)) %
                       secondary_buffer_size);

      if (byte_to_lock > target_cursor) {
        bytes_to_write = secondary_buffer_size - byte_to_lock;
        bytes_to_write += target_cursor;
      } else {
        bytes_to_write = target_cursor - byte_to_lock;
      }

      sound_is_valid = true;
    }

    u32 sample_count = bytes_to_write/bytes_per_sample;
    { // NOTE: Output Sound
      local f32 t_sine = 0.0f;
      u32 tone_volume = 3000;
      u32 tone_hz = 256;
      u32 wave_period = samples_per_sec/tone_hz;

      s16 *dst = samples;
      for (u32 sample_index = 0;
           sample_index < sample_count;
           ++sample_index) {
        f32 sine_value = sin_f32(t_sine);
        s16 sample_value = (s16)(sine_value*tone_volume);
        *dst++ = sample_value;
        *dst++ = sample_value;
        t_sine += tau32/wave_period;
      }
    }

    if (sound_is_valid) {
      VOID *region1;
      VOID *region2;
      DWORD region1_size;
      DWORD region2_size;
      if (SUCCEEDED(IDirectSoundBuffer_Lock(secondary_buffer,
                                            byte_to_lock, bytes_to_write,
                                            &region1, &region1_size,
                                            &region2, &region2_size,
                                            0))) {
        s16 *src = samples;

        s16 *dst1 = (s16 *)region1;
        s16 *dst2 = (s16 *)region2;

        DWORD region1_sample_count = region1_size/bytes_per_sample;
        DWORD region2_sample_count = region2_size/bytes_per_sample;

        for (DWORD sample_index = 0;
             sample_index < region1_sample_count;
             ++sample_index) {
          *dst1++ = *src++;
          *dst1++ = *src++;
          ++running_sample_index;
        }

        for (DWORD sample_index = 0;
             sample_index < region2_sample_count;
             ++sample_index) {
          *dst2++ = *src++;
          *dst2++ = *src++;
          ++running_sample_index;
        }

        IDirectSoundBuffer_Unlock(secondary_buffer,
                                  region1, region1_size,
                                  region2, region2_size);
      }
    }

    u64 cycles_end = __rdtsc();
    u64 dt_cycles = cycles_end - cycles_start;
    f32 dt_mc = dt_cycles/million(1.0f);

    u64 time_end = platform_get_time_us();
    u64 dt_us = time_end - time_start;
    f32 dt_ms = dt_us/thousand(1.0f);
    f32 fps = million(1.0f)/dt_us;

    time_start = time_end;
    cycles_start = cycles_end;

    log_info("%.2ffps, %.2fms, %.2fmc", fps, dt_ms, dt_mc);
    scratch_end(scratch);
  }

  platform_window_close(window);
  platform_core_shutdown();
  return(0);
}
