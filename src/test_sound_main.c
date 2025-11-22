#define KRUEGER_PLATFORM_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#include <dsound.h>
#pragma comment(lib, "dsound.lib")

#define DIRECT_SOUND_CREATE(x) HRESULT WINAPI x(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create_proc);

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

      LPDIRECTSOUNDBUFFER secondary_buffer;
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

int
main(void) {
  platform_core_init();
  platform_graphics_init();

  Platform_Handle window = platform_window_open(str8_lit("sound test"), 800, 600);
  platform_window_show(window);

  win32_direct_sound_init(window);

  Arena misc_arena = arena_alloc(MB(64));
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
