#define KRUEGER_PLATFORM_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#include <dsound.h>
#pragma comment(lib, "dsound.lib")

#define DIRECT_SOUND_CREATE(x) HRESULT WINAPI x(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create_proc);

global LPDIRECTSOUNDBUFFER secondary_buffer;

internal void
win32_direct_sound_init(Platform_Handle window) {
  String8 lib = str8_lit("dsound.dll");
  Platform_Handle dsound_lib = platform_library_open(lib);
  if (!platform_handle_is_null(dsound_lib)) {
    direct_sound_create_proc *direct_sound_create = (direct_sound_create_proc *)
      platform_library_load_proc(dsound_lib, "DirectSoundCreate");

    LPDIRECTSOUND direct_sound;
    if (SUCCEEDED(direct_sound_create(0, &direct_sound, 0))) {
      WAVEFORMATEX wave_format = {0};
      wave_format.wFormatTag = WAVE_FORMAT_PCM;
      wave_format.nChannels = 2;
      wave_format.wBitsPerSample = 16;
      wave_format.nBlockAlign = (wave_format.wBitsPerSample/8)*wave_format.nChannels;
      wave_format.nSamplesPerSec = 48000;
      wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec*wave_format.nBlockAlign;

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
          HRESULT hresult = IDirectSoundBuffer_SetFormat(primary_buffer, &wave_format);
          if (SUCCEEDED(hresult)) {
            log_info("%s: direct sound: primary buffer format was set", __func__);
          } else {
            log_error("%s: direct sound: failed to set primary buffer format", __func__);
          }
        }
      }

      DSBUFFERDESC buffer_desc = {0};
      buffer_desc.dwSize = sizeof(buffer_desc);
      buffer_desc.dwFlags = 0;
      buffer_desc.dwBufferBytes = 48000*sizeof(s16)*wave_format.nChannels;
      buffer_desc.lpwfxFormat = &wave_format;

      HRESULT hresult = IDirectSound_CreateSoundBuffer(direct_sound, &buffer_desc, &secondary_buffer, 0);
      if (SUCCEEDED(hresult)) {
        log_info("%s: direct sound: secondary buffer created successfully", __func__);
      } else {
        log_error("%s: direct sound: failed to create secondary buffer", __func__);
      }
    }
  } else {
    log_error("%s: failed to open library: [%s]", __func__, lib.str);
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

typedef struct {
  u16 num_channels;
  u16 bits_per_sample;
  u16 block_align;
  u32 sample_rate;
  u32 byte_rate;
} Audio_File;

internal void
load_wave(Arena *arena, Temp scratch, String8 file_path) {
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
}

int
main(void) {
  platform_core_init();
  platform_graphics_init();
  
  Arena main_arena = arena_alloc(MB(64));
  Arena misc_arena = arena_alloc(MB(64));

  Temp scratch = temp_begin(&misc_arena);
  load_wave(&main_arena, scratch, str8_lit("../res/test.wav"));
  temp_end(scratch);

  Platform_Handle window = platform_window_open(str8_lit("sound test"), 800, 600);
  platform_window_show(window);

  u32 samples_per_sec = 48000;
  u32 num_channels = 2;
  u32 bytes_per_sample = sizeof(s16)*num_channels;
  u32 secondary_buffer_size = samples_per_sec*bytes_per_sample;
  u32 latency_sample_count = samples_per_sec/15;
  u32 running_sample_index = 0;

  s16 *samples = platform_reserve(secondary_buffer_size);
  platform_commit(samples, secondary_buffer_size);

  win32_direct_sound_init(window);

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

  Arena event_arena = arena_alloc(MB(64));
  u64 time_start = platform_get_time_us();
  u64 cycles_start = __rdtsc();

  for (b32 quit = false; !quit;) {
    Temp temp = temp_begin(&event_arena);
    Platform_Event_List event_list = platform_get_event_list(temp.arena);
    for (Platform_Event *event = event_list.first; event != 0; event = event->next) {
      switch (event->type) {
        case PLATFORM_EVENT_WINDOW_CLOSE: {
          quit = true;
        } break;
      }
    }
    temp_end(temp);
    
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

    temp = temp_begin(&misc_arena);
    String8 text = str8_fmt(temp.arena, "%.2ffps, %.2fms, %.2fmc", fps, dt_ms, dt_mc);
    log_info((char *)text.str);
    temp_end(temp);
  }

  platform_window_close(window);
  platform_core_shutdown();
  return(0);
}
