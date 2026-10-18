#ifndef JC_AUDIO_MIXER_H
#define JC_AUDIO_MIXER_H
#include "resource.h"
/* Original resources: PCM mono unsigned 8-bit, 11025 Hz (all 23 WAVE headers).
 * Samples are decoded by resource.c and borrowed until audio shutdown. */
#define JC_AUDIO_SOURCE_RATE 11025
typedef struct {
  const JCSound *sounds, *voice;
  size_t count;
  double position;
  unsigned rate;
  float volume;
  bool muted;
} JCAudioMixer;
bool jc_audio_mixer_play(JCAudioMixer *, uint32_t id);
void jc_audio_mixer_render(JCAudioMixer *, float *, size_t frames,
                           unsigned channels);
#endif
