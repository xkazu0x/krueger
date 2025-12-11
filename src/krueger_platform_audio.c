#ifndef KRUEGER_PLATFORM_AUDIO_C
#define KRUEGER_PLATFORM_AUDIO_C

//////////////////////////
// NOTE: Windows Functions

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
      _platform_audio_state.callback(samples, num_samples, _platform_audio_state.sample_rate, _platform_audio_state.user_data);
    }

    IAudioRenderClient_ReleaseBuffer(_platform_audio_state.backend.render_client, fill_frame_count, 0);
  }
  return(0);
}

internal void
_win32_wasapi_backend_init(void) {
  _win32_check(CoInitializeEx(0, COINIT_MULTITHREADED));
  _platform_audio_state.backend.buffer_end_event = CreateEvent(0, 0, 0, 0);

  _win32_check(CoCreateInstance(&_win32_CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, &_win32_IID_IMMDeviceEnumerator, &_platform_audio_state.backend.device_enumerator));
  _win32_check(IMMDeviceEnumerator_GetDefaultAudioEndpoint(_platform_audio_state.backend.device_enumerator, eRender, eConsole, &_platform_audio_state.backend.device));
  _win32_check(IMMDevice_Activate(_platform_audio_state.backend.device, &_win32_IID_IAudioClient, CLSCTX_ALL, 0, &_platform_audio_state.backend.audio_client));

  WAVEFORMATEX audio_format = {0};
  audio_format.wFormatTag = WAVE_FORMAT_PCM;
  audio_format.nChannels = (WORD)_platform_audio_state.num_channels;
  audio_format.nSamplesPerSec = _platform_audio_state.sample_rate;
  audio_format.wBitsPerSample = BITS_PER_SAMPLE;
  audio_format.nBlockAlign = (audio_format.nChannels*audio_format.wBitsPerSample)/8;
  audio_format.nAvgBytesPerSec = audio_format.nSamplesPerSec*audio_format.nBlockAlign;

  u32 audio_client_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
  _win32_check(IAudioClient_Initialize(_platform_audio_state.backend.audio_client, AUDCLNT_SHAREMODE_SHARED, audio_client_flags, 0, 0, &audio_format, 0));
  _win32_check(IAudioClient_GetService(_platform_audio_state.backend.audio_client, &_win32_IID_IAudioRenderClient, &_platform_audio_state.backend.render_client));
  _win32_check(IAudioClient_SetEventHandle(_platform_audio_state.backend.audio_client, _platform_audio_state.backend.buffer_end_event));

  _platform_audio_state.backend.thread_handle = CreateThread(0, 0, _win32_wasapi_thread_proc, 0, 0, 0);
}

internal void
_win32_wasapi_backend_shutdown(void) {
  // SetEvent(_platform_audio_state.backend.buffer_end_event);
  // WaitForSingleObject(_platform_audio_state.backend.thread_handle, INFINITE);
  CloseHandle(_platform_audio_state.backend.thread_handle);
  IAudioClient_Stop(_platform_audio_state.backend.audio_client);

  IAudioRenderClient_Release(_platform_audio_state.backend.render_client);
  IAudioClient_Release(_platform_audio_state.backend.audio_client);
  IMMDevice_Release(_platform_audio_state.backend.device);
  IMMDeviceEnumerator_Release(_platform_audio_state.backend.device_enumerator);

  CloseHandle(_platform_audio_state.backend.buffer_end_event);
  CoUninitialize();
}

///////////////////////////
// NOTE: Platform Functions

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

#endif // KRUEGER_PLATFORM_AUDIO_C
