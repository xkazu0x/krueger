#ifndef KRUEGER_PLATFORM_AUDIO_C
#define KRUEGER_PLATFORM_AUDIO_C

/////////////////////////
// NOTE: Platform Helpers

internal void
_platform_setup_audio_desc(Platform_Audio_Desc *desc) {
  _platform_audio_desc.callback = desc->callback;
  _platform_audio_desc.user_data = desc->user_data;
  _platform_audio_desc.sample_rate = null_def(desc->sample_rate, audio_default_sample_rate);
  _platform_audio_desc.num_channels = null_def(desc->num_channels, audio_default_num_channels);
}

#endif // KRUEGER_PLATFORM_AUDIO_C
