#include "audio_mixer.h"
#include <string.h>
bool jc_audio_mixer_play(JCAudioMixer *m, uint32_t id) {
  for (size_t i = 0; i < m->count; ++i)
    if (m->sounds[i].id == id && m->sounds[i].count) {
      m->voice = &m->sounds[i];
      m->position = 0;
      return true;
    }
  return false;
}
void jc_audio_mixer_render(JCAudioMixer *m, float *out, size_t frames,
                           unsigned channels) {
  memset(out, 0, frames * channels * sizeof *out);
  if (!m->rate || !channels)
    return;
  const double step = (double)JC_AUDIO_SOURCE_RATE / m->rate;
  for (size_t i = 0; i < frames && m->voice; ++i) {
    const JCSound *s = m->voice;
    if (m->position >= s->count) {
      m->voice = NULL;
      break;
    }
    size_t p = (size_t)m->position;
    float a = s->samples[p];
    float b = p + 1 < s->count ? s->samples[p + 1] : a;
    float v = (a + (b - a) * (float)(m->position - p)) * m->volume;
    if (m->muted)
      v = 0;
    for (unsigned c = 0; c < channels; ++c)
      out[i * channels + c] = v;
    m->position += step;
  }
}
