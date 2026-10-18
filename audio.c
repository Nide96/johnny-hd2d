#define SOKOL_AUDIO_IMPL
#include "sokol_audio.h"
#include "sokol_log.h"
#include "audio.h"
#include "audio_mixer.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct JCAudio {
  pthread_mutex_t lock;
  JCAudioMixer mixer;
  uint64_t requests, played, callbacks, nonzero;
};
/* No allocation, file access or VM calls on the audio thread. The short lock
 * protects a single voice; assets outlive saudio_shutdown's thread join. */
static void stream(float *out, int frames, int channels, void *user) {
  JCAudio *a = user;
  pthread_mutex_lock(&a->lock);
  jc_audio_mixer_render(&a->mixer, out, (size_t)frames, (unsigned)channels);
  ++a->callbacks;
  for (size_t i = 0; i < (size_t)frames * (unsigned)channels; ++i)
    if (out[i] != 0)
      ++a->nonzero;
  pthread_mutex_unlock(&a->lock);
}
JCAudio *jc_audio_create(const JCExe *exe, unsigned volume, bool muted) {
  JCAudio *a = calloc(1, sizeof *a);
  if (!a)
    return NULL;
  if (pthread_mutex_init(&a->lock, NULL)) {
    free(a);
    return NULL;
  }
  a->mixer = (JCAudioMixer){.sounds = exe->sounds,
                            .count = exe->sound_count,
                            .volume = (volume > 100 ? 100 : volume) / 100.0f,
                            .muted = muted};
  saudio_setup(&(saudio_desc){.sample_rate = 48000,
                              .num_channels = 2,
                              .buffer_frames = 1024,
                              .stream_userdata_cb = stream,
                              .user_data = a,
                              .logger.func = slog_func});
  if (!saudio_isvalid()) {
    fputs("Audio unavailable: continuing without sound\n", stderr);
    jc_audio_destroy(a);
    return NULL;
  }
  pthread_mutex_lock(&a->lock);
  a->mixer.rate = (unsigned)saudio_sample_rate();
  pthread_mutex_unlock(&a->lock);
  fprintf(stderr,
          "Audio: ALSA, accepted %d Hz / %d channels / %d frames; "
          "%zu effects, volume %u%%%s\n",
          saudio_sample_rate(), saudio_channels(), saudio_buffer_frames(),
          exe->sound_count, volume, muted ? " (muted)" : "");
  return a;
}
void jc_audio_events(JCAudio *a, const uint32_t *ids, size_t count,
                     bool enabled) {
  if (!a)
    return;
  pthread_mutex_lock(&a->lock);
  if (!enabled)
    a->mixer.voice = NULL;
  else
    for (size_t i = 0; i < count; ++i) {
      ++a->requests;
      if (jc_audio_mixer_play(&a->mixer, ids[i]))
        ++a->played;
    }
  pthread_mutex_unlock(&a->lock);
}
void jc_audio_toggle_mute(JCAudio *a) {
  if (!a)
    return;
  pthread_mutex_lock(&a->lock);
  a->mixer.muted = !a->mixer.muted;
  bool muted = a->mixer.muted;
  pthread_mutex_unlock(&a->lock);
  fprintf(stderr, "Audio: %s\n", muted ? "muted" : "unmuted");
}
void jc_audio_destroy(JCAudio *a) {
  if (!a)
    return;
  saudio_shutdown();
  fprintf(stderr,
          "Audio: %llu requests, %llu recognized, %llu callbacks, "
          "%llu nonzero output samples\n",
          (unsigned long long)a->requests, (unsigned long long)a->played,
          (unsigned long long)a->callbacks, (unsigned long long)a->nonzero);
  pthread_mutex_destroy(&a->lock);
  free(a);
}
