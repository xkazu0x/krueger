#ifndef KRUEGER_PLATFORM_AUDIO_WIN32_C
#define KRUEGER_PLATFORM_AUDIO_WIN32_C

//////////////////////////
// NOTE: Windows Functions

internal DWORD WINAPI
_win32_wasapi_thread_proc(LPVOID param) {
  u32 buffer_frame_count;
  IAudioClient_GetBufferSize(_win32_audio_state.audio_client, &buffer_frame_count);
  IAudioClient_Start(_win32_audio_state.audio_client);
  for (;;) {
    WaitForSingleObject(_win32_audio_state.buffer_end_event, INFINITE);
    u32 padding_frame_count;
    IAudioClient_GetCurrentPadding(_win32_audio_state.audio_client, &padding_frame_count);
    u32 num_frames = buffer_frame_count - padding_frame_count;
    s16 *frames;
    IAudioRenderClient_GetBuffer(_win32_audio_state.render_client, num_frames, (BYTE **)&frames);
    _platform_audio_desc.callback(frames, num_frames, _platform_audio_desc.user_data);
    IAudioRenderClient_ReleaseBuffer(_win32_audio_state.render_client, num_frames, 0);
  }
  return(0);
}

internal void
_win32_wasapi_backend_init(void) {
  _win32_check(CoInitializeEx(0, COINIT_MULTITHREADED));
  _win32_audio_state.buffer_end_event = CreateEvent(0, 0, 0, 0);

  _win32_check(CoCreateInstance(&_win32_CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, &_win32_IID_IMMDeviceEnumerator, &_win32_audio_state.device_enumerator));
  _win32_check(IMMDeviceEnumerator_GetDefaultAudioEndpoint(_win32_audio_state.device_enumerator, eRender, eConsole, &_win32_audio_state.device));
  _win32_check(IMMDevice_Activate(_win32_audio_state.device, &_win32_IID_IAudioClient, CLSCTX_ALL, 0, &_win32_audio_state.audio_client));

  WAVEFORMATEX audio_format = {0};
  audio_format.wFormatTag = WAVE_FORMAT_PCM;
  audio_format.nChannels = (WORD)_platform_audio_desc.num_channels;
  audio_format.nSamplesPerSec = _platform_audio_desc.sample_rate;
  audio_format.wBitsPerSample = PLATFORM_AUDIO_BITS_PER_SAMPLE;
  audio_format.nBlockAlign = (audio_format.nChannels*audio_format.wBitsPerSample)/8;
  audio_format.nAvgBytesPerSec = audio_format.nSamplesPerSec*audio_format.nBlockAlign;

  u32 audio_client_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
  _win32_check(IAudioClient_Initialize(_win32_audio_state.audio_client, AUDCLNT_SHAREMODE_SHARED, audio_client_flags, 0, 0, &audio_format, 0));
  _win32_check(IAudioClient_GetService(_win32_audio_state.audio_client, &_win32_IID_IAudioRenderClient, &_win32_audio_state.render_client));
  _win32_check(IAudioClient_SetEventHandle(_win32_audio_state.audio_client, _win32_audio_state.buffer_end_event));

  _win32_audio_state.thread_handle = CreateThread(0, 0, _win32_wasapi_thread_proc, 0, 0, 0);
}

internal void
_win32_wasapi_backend_shutdown(void) {
  CloseHandle(_win32_audio_state.thread_handle);
  IAudioClient_Stop(_win32_audio_state.audio_client);

  IAudioRenderClient_Release(_win32_audio_state.render_client);
  IAudioClient_Release(_win32_audio_state.audio_client);
  IMMDevice_Release(_win32_audio_state.device);
  IMMDeviceEnumerator_Release(_win32_audio_state.device_enumerator);

  CloseHandle(_win32_audio_state.buffer_end_event);
  CoUninitialize();
}

///////////////////////////
// NOTE: Platform Functions

internal void
platform_audio_init(Platform_Audio_Desc *desc) {
  _platform_setup_audio_desc(desc);
  _win32_wasapi_backend_init();
}

internal void
platform_audio_shutdown(void) {
  _win32_wasapi_backend_shutdown();
}

#endif // KRUEGER_PLATFORM_AUDIO_WIN32_C
