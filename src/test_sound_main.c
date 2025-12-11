#define PLATFORM_FEATURE_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"
#include "starfighter.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#include <mmeapi.h>
#include <dsound.h>
#pragma comment(lib, "dsound")

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

typedef struct {
  u32 sample_rate;
  u32 byte_rate;
  u16 bits_per_sample;
  u16 num_channels;
  u16 block_align;
} Audio_Format;

internal Audio_Format
make_audio_format(u32 sample_rate, u16 bits_per_sample, u16 num_channels) {
  Audio_Format result = {0};
  result.bits_per_sample = bits_per_sample;
  result.num_channels = num_channels;
  result.block_align = (bits_per_sample*num_channels)/8;
  result.sample_rate = sample_rate;
  result.byte_rate = sample_rate*result.block_align;
  return(result);
}

internal u32
size_from_audio_format(Audio_Format format) {
  u32 result = format.sample_rate*(format.bits_per_sample/8)*format.num_channels;
  return(result);
}

typedef struct {
  Audio_Format format;
  u32 size;
  s16 *data;
} Audio_File;

typedef struct Physical_Device Physical_Device;
struct Physical_Device {
  Physical_Device *next;
  Physical_Device *prev;
  Platform_Handle h;
  String8 name;
  u32 id;
};

typedef struct {
  Physical_Device *first;
  Physical_Device *last;
  u32 count;
} Physical_Device_List;

typedef struct Win32_Audio_Device Win32_Audio_Device;
struct Win32_Audio_Device {
  Win32_Audio_Device *next;
  Win32_Audio_Device *prev;
  LPDIRECTSOUND direct_sound;
  LPDIRECTSOUNDBUFFER primary_buffer;
  LPDIRECTSOUNDBUFFER secondary_buffer;
};

typedef struct {
  Arena *arena;
  Physical_Device_List playback_device_list;
  Win32_Audio_Device *first_device;
  Win32_Audio_Device *last_device;
  Win32_Audio_Device *free_device;
} Win32_Audio_State;

global Win32_Audio_State *win32_audio_state;

internal Physical_Device *
push_physical_device(Arena *arena, Physical_Device_List *list) {
  Physical_Device *result = push_array(arena, Physical_Device, 1);
  mem_zero_struct(result);
  queue_push(list->first, list->last, result);
  list->count += 1;
  return(result);
}

BOOL CALLBACK
win32_direct_sound_enumerate_callback(LPGUID guid,
                                      LPCSTR cstr_description,
                                      LPCSTR cstr_module,
                                      LPVOID context) {
  Arena *arena = win32_audio_state->arena;
  Physical_Device_List *list = &win32_audio_state->playback_device_list;

  Physical_Device *device = push_physical_device(arena, list);
  device->h.ptr[0] = (uxx)guid;
  device->name = str8_copy(arena, str8_cstr((char *)cstr_description));
  device->id = list->count - 1;

  BOOL result = TRUE;
  return(result);
}

internal Win32_Audio_Device *
win32_audio_device_alloc(void) {
  Win32_Audio_Device *result = win32_audio_state->free_device;
  if (result) {
    stack_pop(win32_audio_state->free_device);
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
platform_audio_init(void) {
  Arena *arena = arena_alloc();
  win32_audio_state = push_array(arena, Win32_Audio_State, 1);
  win32_audio_state->arena = arena;
  DirectSoundEnumerate(win32_direct_sound_enumerate_callback, 0);
}

internal Physical_Device_List
platform_get_playback_device_list(void) {
  return(win32_audio_state->playback_device_list);
}

internal Physical_Device *
platform_get_playback_device(u32 index) {
  assert(index < win32_audio_state->playback_device_list.count);
  Physical_Device *result = 0;
  for (Physical_Device *device = win32_audio_state->playback_device_list.first;
       device != 0;
       device = device->next) {
    if (device->id == index) {
      result = device;
      break;
    }
  }
  return(result);
}

internal Platform_Handle
platform_audio_device_open(Physical_Device *physical_device,
                           Audio_Format format,
                           Platform_Handle window) {
  Platform_Handle result = {0};
  LPGUID guid = (LPGUID)physical_device->h.ptr[0];

  LPDIRECTSOUND direct_sound;
  if (SUCCEEDED(DirectSoundCreate(guid, &direct_sound, 0))) {
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
      DSBUFFERDESC primary_buffer_desc = {0};
      primary_buffer_desc.dwSize = sizeof(primary_buffer_desc);
      primary_buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;

      LPDIRECTSOUNDBUFFER primary_buffer;
      if (SUCCEEDED(IDirectSound_CreateSoundBuffer(direct_sound,
                                                   &primary_buffer_desc,
                                                   &primary_buffer,
                                                   0))) {
        HRESULT hr = IDirectSoundBuffer_SetFormat(primary_buffer, &wave_format);
        if (!SUCCEEDED(hr)) {
          log_error("%s: direct sound: failed to set primary buffer format", __func__);
        }

        DSBUFFERDESC secondary_buffer_desc = {0};
        secondary_buffer_desc.dwSize = sizeof(secondary_buffer_desc);
        secondary_buffer_desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
        secondary_buffer_desc.dwBufferBytes = size_from_audio_format(format);
        secondary_buffer_desc.lpwfxFormat = &wave_format;

        LPDIRECTSOUNDBUFFER secondary_buffer;
        hr = IDirectSound_CreateSoundBuffer(direct_sound,
                                            &secondary_buffer_desc,
                                            &secondary_buffer,
                                            0);
        if (SUCCEEDED(hr)) {
          Win32_Audio_Device *device = win32_audio_device_alloc();
          device->direct_sound = direct_sound;
          device->primary_buffer = primary_buffer;
          device->secondary_buffer = secondary_buffer;

          result.ptr[0] = (uxx)device;
        } else {
          log_error("%s: direct sound: failed to create secondary buffer", __func__);
        }
      } else {
        log_error("%s: direct sound: failed to create primary buffer", __func__);
      }
    }
  } else {
    log_error("%s: direct sound: failed to open audio device", __func__);
  }
  return(result);
}

internal void
platform_audio_device_close(Platform_Handle handle) {
  Win32_Audio_Device *device = (Win32_Audio_Device *)handle.ptr[0];
  IDirectSoundBuffer_Release(device->secondary_buffer);
  IDirectSoundBuffer_Release(device->primary_buffer);
  IDirectSound_Release(device->direct_sound);
  dll_remove(win32_audio_state->first_device,
             win32_audio_state->last_device,
             device);
  stack_push(win32_audio_state->free_device, device);
}

internal void
output_sound(s16 *samples, u32 sample_count, u32 sample_rate) {
  local f32 t_sine = 0.0f;
  u32 tone_volume = 4000;
  u32 tone_hz = 300;
  u32 wave_period = sample_rate/tone_hz;

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

internal void
output_music(s16 *samples, u32 sample_count, Audio *music) {
  local u32 music_index = 0;

  u32 music_sample_count = music->size/2;
  s16 *src = (s16 *)music->data;

  s16 *dst = samples;
  for (u32 sample_index = 0;
       sample_index < sample_count;
       ++sample_index) {
    *dst++ = src[music_index++ % music_sample_count];
    *dst++ = src[music_index++ % music_sample_count];
  }
}

internal void
entry_point(int argc, char **argv) {
  platform_audio_init();

  Arena *arena = arena_alloc();
  Audio audio = load_wav(arena, str8_lit("../res/fireflies.wav"));
  
  u32 window_w = 800;
  u32 window_h = 600;

  Image image = image_alloc(window_w, window_h);

  Platform_Handle window = platform_window_open(str8_lit("sound test"), 800, 600);
  platform_window_show(window);

  Platform_Graphics_Info graphics_info = platform_get_graphics_info();
  f32 refresh_rate = graphics_info.refresh_rate;
  f32 target_us_per_frame = million(1.0f)/refresh_rate;
  f32 target_seconds_per_frame = 1.0f/refresh_rate;

  Physical_Device_List list = platform_get_playback_device_list();
  for (Physical_Device *device = list.first;
      device != 0;
      device = device->next) {
    log_info("[%d] %s", device->id, device->name.str);
  }

  Audio_Format audio_format = make_audio_format(48000, 16, 2);
  Physical_Device *playback_device = platform_get_playback_device(0);
  Platform_Handle audio_device = platform_audio_device_open(playback_device,
                                                            audio_format,
                                                            window);

  Win32_Audio_Device *device = (Win32_Audio_Device *)audio_device.ptr[0];

  u32 running_sample_index = 0;
  u32 samples_per_second = audio_format.sample_rate;
  u32 bytes_per_sample = audio_format.block_align;
  u32 secondary_buffer_size = size_from_audio_format(audio_format);

  u32 safety_bytes = (samples_per_second*bytes_per_sample/(u32)refresh_rate);

  s16 *samples = platform_reserve(secondary_buffer_size);
  platform_commit(samples, secondary_buffer_size);

  { // NOTE: Clear Sound Buffer
    VOID *region1;
    VOID *region2;
    DWORD region1_size;
    DWORD region2_size;
    if (SUCCEEDED(IDirectSoundBuffer_Lock(device->secondary_buffer,
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
      IDirectSoundBuffer_Unlock(device->secondary_buffer,
                                region1, region1_size,
                                region2, region2_size);
    }
  }

  IDirectSoundBuffer_Play(device->secondary_buffer, 0, 0, DSBPLAY_LOOPING);

  b32 sound_is_valid = false;
  
  u64 flip_time = platform_get_time_us();
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
    scratch_end(scratch);
    if (quit) break;

    { // NOTE: Render
      u32 *row = image.pixels;
      for (u32 y = 0; y < image.height; ++y) {
        u32 *pixels = row;
        for (u32 x = 0; x < image.width; ++x) {
          u32 color = ((y%255) << 8) | ((x%255) << 0);
          *pixels++ = color;
        }
        row += image.pitch;
      }
    }

    u64 audio_time = platform_get_time_us();
    f32 from_begin_to_audio_seconds = (flip_time - audio_time)/million(1.0f);

    DWORD play_cursor, write_cursor;
    if (IDirectSoundBuffer_GetCurrentPosition(device->secondary_buffer,
                                              &play_cursor, &write_cursor) == DS_OK) {
      if (!sound_is_valid) {
        running_sample_index = write_cursor/bytes_per_sample;
        sound_is_valid = true;
      }

      DWORD byte_to_lock =
        ((running_sample_index*bytes_per_sample) % secondary_buffer_size);
  
      DWORD expected_sound_bytes_per_frame =
        (samples_per_second*bytes_per_sample)/(u32)refresh_rate;

      f32 seconds_left_until_flip = target_seconds_per_frame - from_begin_to_audio_seconds;
      DWORD expected_bytes_until_flip = (DWORD)((seconds_left_until_flip/target_seconds_per_frame)*expected_sound_bytes_per_frame);

      DWORD expected_frame_boundary_byte = play_cursor + expected_bytes_until_flip;

      DWORD safe_write_cursor = write_cursor;
      if (safe_write_cursor < play_cursor) {
        safe_write_cursor += secondary_buffer_size;
      }
      assert(safe_write_cursor >= play_cursor);
      safe_write_cursor += safety_bytes;

      b32 audio_device_is_low_latency = (safe_write_cursor < expected_frame_boundary_byte);

      DWORD target_cursor = 0;
      if (audio_device_is_low_latency) {
        target_cursor = expected_frame_boundary_byte + expected_sound_bytes_per_frame;
      } else {
        target_cursor = write_cursor + expected_sound_bytes_per_frame + safety_bytes;
      }
      target_cursor = target_cursor % secondary_buffer_size;

      DWORD bytes_to_write = 0;
      if (byte_to_lock > target_cursor) {
        bytes_to_write = secondary_buffer_size - byte_to_lock;
        bytes_to_write += target_cursor;
      } else {
        bytes_to_write = target_cursor - byte_to_lock;
      }

      u32 sample_count = bytes_to_write/bytes_per_sample;
      output_sound(samples, sample_count, samples_per_second);
      // output_music(samples, sample_count, &audio);

#if 0
      DWORD play_cursor = 0;
      DWORD write_cursor = 0;
      IDirectSoundBuffer_GetCurrentPosition(device->secondary_buffer,
                                            &play_cursor,
                                            &write_cursor);

      DWORD unwrapped_write_cursor = write_cursor;
      if (unwrapped_write_cursor < play_cursor) {
        unwrapped_write_cursor += secondary_buffer_size;
      }

      DWORD audio_latency_bytes = unwrapped_write_cursor - play_cursor;
      f32 audio_latency_seconds = 
        (((f32)audio_latency_bytes/(f32)bytes_per_sample)/
         (f32)samples_per_second);
      log_info("pc:%d wc:%d dt:%d (%.2fs)",
               play_cursor, write_cursor, audio_latency_bytes, audio_latency_seconds);
#endif

      VOID *region1;
      VOID *region2;
      DWORD region1_size;
      DWORD region2_size;
      if (SUCCEEDED(IDirectSoundBuffer_Lock(device->secondary_buffer,
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

        IDirectSoundBuffer_Unlock(device->secondary_buffer,
                                  region1, region1_size,
                                  region2, region2_size);
      }
    } else {
      sound_is_valid = false;
    }

    { // NOTE: wait to flip
      u64 time_end = platform_get_time_us();
      u64 us_per_frame = time_end - time_start;
      f32 work_us_per_frame = (f32)us_per_frame;

      if (work_us_per_frame < target_us_per_frame) {
        f32 dt_us = target_us_per_frame - work_us_per_frame;
        u32 sleep_ms = (u32)(dt_us/thousand(1.0f));
        if (sleep_ms > 0) platform_sleep_ms(sleep_ms);

        // u64 test_end = platform_get_time_us();
        // f32 test_us = (f32)(test_end - time_start);
        // assert(test_us < target_us_per_frame);

        while (work_us_per_frame < target_us_per_frame) {
          time_end = platform_get_time_us();
          us_per_frame = time_end - time_start;
          work_us_per_frame = (f32)us_per_frame;
        }
      } else {
        // TODO: missed frame rate!
      }
    }

    u64 cycles_end = __rdtsc();
    u64 time_end = platform_get_time_us();

#if 1
    { // NOTE: print time
      u64 cycles_per_frame = cycles_end - cycles_start;
      f32 mc = cycles_per_frame/million(1.0f);
      
      u64 dt_us = time_end - time_start;
      f32 ms = dt_us/thousand(1.0f);
      f32 fps = million(1.0f)/dt_us;

      log_info("%.2ffps, %.2fms, %.2fmc", fps, ms, mc);
    }
#endif

    time_start = time_end;
    cycles_start = cycles_end;

    platform_window_blit(window, image.pixels, image.width, image.height);

    flip_time = platform_get_time_us();
  }

  platform_audio_device_close(audio_device);
  platform_window_close(window);
}
