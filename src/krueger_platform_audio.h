#ifndef KRUEGER_PLATFORM_AUDIO_H
#define KRUEGER_PLATFORM_AUDIO_H

#define replace_null(val, is_null) (((val) == 0) ? (is_null) : (val))
#define _win32_check(x) assert((x) == S_OK)
#define BITS_PER_SAMPLE 16

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

typedef struct Platform_Audio_Desc {
  u32 sample_rate;
  u32 num_channels;
  void (*callback)(s16 *samples, u32 num_samples, u32 sample_rate, void *user_data);
  void *user_data;
} Platform_Audio_Desc;

typedef struct Win32_Audio_Backend {
  HANDLE thread_handle;
  HANDLE buffer_end_event;
  IMMDeviceEnumerator *device_enumerator;
  IMMDevice *device;
  IAudioClient *audio_client;
  IAudioRenderClient *render_client;
} Win32_Audio_Backend;

typedef struct Win32_Audio_Backend Platform_Audio_Backend;
typedef struct Platform_Audio_State {
  u32 sample_rate;
  u32 num_channels;
  void (*callback)(s16 *samples, u32 num_samples, u32 sample_rate, void *user_data);
  void *user_data;
  Platform_Audio_Backend backend;
} Platform_Audio_State;

global Platform_Audio_State _platform_audio_state;
global const u32 audio_default_sample_rate = 44100;
global const u16 audio_default_num_channels = 1;

internal DWORD WINAPI _win32_wasapi_thread_proc(LPVOID param);
internal void _win32_wasapi_backend_init(void);
internal void _win32_wasapi_backend_shutdown(void);

internal void platform_audio_init(Platform_Audio_Desc *desc);
internal void platform_audio_shutdown(void);

#endif // KRUEGER_PLATFORM_AUDIO_H
