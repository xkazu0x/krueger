#ifndef KRUEGER_PLATFORM_AUDIO_C
#define KRUEGER_PLATFORM_AUDIO_C

/////////////////////////
// NOTE: Platform Helpers

internal
PLATFORM_AUDIO_CALLBACK(_platform_audio_cb_stub) {
}

internal void
_platform_setup_audio_desc(Platform_Audio_Desc *desc) {
  _platform_audio_desc.sample_rate = null_def(desc->sample_rate, _audio_default_sample_rate);
  _platform_audio_desc.num_channels = null_def(desc->num_channels, _audio_default_num_channels);
  _platform_audio_desc.callback = null_def(desc->callback, _platform_audio_cb_stub);
  _platform_audio_desc.user_data = desc->user_data;
}

#endif // KRUEGER_PLATFORM_AUDIO_C
