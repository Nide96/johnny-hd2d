#include "audio_mixer.h"
#include "vm_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void close_to(float a, float b) { assert(fabsf(a - b) < 1e-6f); }
typedef struct {
  uint64_t calls, events, hash;
  bool enabled;
} Events;
static void observe(void *user, const uint32_t *ids, size_t count,
                    bool enabled) {
  Events *e = user;
  ++e->calls;
  e->enabled = enabled;
  e->events += count;
  for (size_t i = 0; i < count; ++i)
    e->hash = (e->hash ^ ids[i]) * UINT64_C(1099511628211);
}
int main(int argc, char **argv) {
  float up[] = {0, 1, 0, -1}, down[] = {-0.5f, -0.25f};
  JCSound sounds[] = {{1, up, 4}, {2, down, 2}, {3, NULL, 0}};
  JCAudioMixer m = {.sounds = sounds, .count = 3, .rate = 22050, .volume = 1};
  float out[32];
  jc_audio_mixer_render(&m, out, 16, 2);
  for (size_t i = 0; i < 32; ++i)
    close_to(out[i], 0);
  assert(jc_audio_mixer_play(&m, 1));
  jc_audio_mixer_render(&m, out, 4, 2);
  close_to(out[0], 0);
  close_to(out[2], 0.5f);
  close_to(out[4], 1);
  close_to(out[6], 0.5f);
  for (size_t i = 0; i < 4; ++i)
    close_to(out[2 * i], out[2 * i + 1]);
  assert(!jc_audio_mixer_play(&m, 999) && m.position == 2);
  assert(!jc_audio_mixer_play(&m, 3));
  assert(jc_audio_mixer_play(&m, 2)); /* Cuts first voice immediately. */
  m.volume = 0.5f;
  jc_audio_mixer_render(&m, out, 1, 1);
  close_to(out[0], -0.25f);
  m.muted = true;
  jc_audio_mixer_render(&m, out, 2, 2);
  for (size_t i = 0; i < 4; ++i)
    close_to(out[i], 0);
  assert(m.position == 1.5); /* Muting keeps time, never replays old events. */
  m.muted = false;
  jc_audio_mixer_render(&m, out, 16, 1);
  close_to(out[0], -0.125f);
  close_to(out[1], 0);
  assert(!m.voice);
  /* Chunk boundaries at a noninteger device ratio must not reset phase. */
  m.rate = 48000;
  m.volume = 1;
  assert(jc_audio_mixer_play(&m, 1));
  JCAudioMixer n = m;
  float split[32];
  jc_audio_mixer_render(&m, out, 32, 1);
  jc_audio_mixer_render(&n, split, 7, 1);
  jc_audio_mixer_render(&n, split + 7, 25, 1);
  assert(!memcmp(out, split, sizeof out));
  puts(
      "audio mixer: resampling, stereo, interruption, mute, volume, bounds OK");

  VmCore *a = calloc(1, sizeof *a), *b = calloc(1, sizeof *b);
  assert(a && b);
  JCClock clock = {1993, 6, 1, 17, 0};
  JCConfig cfg = jc_config_default();
  const char *dir = argc > 1 ? argv[1] : "assets/original";
  uint64_t total = 0;
  for (int scene = 1; scene < 80; ++scene) {
    assert(vm_init(a, dir, 4242, &clock, &cfg, scene));
    assert(vm_init(b, dir, 4242, &clock, &cfg, scene));
    Events observed = {0}, expected = {0};
    a->sound_observer = observe;
    a->sound_user = &observed;
    for (unsigned i = 0; i < 1500 && !jc_engine_scenes_done(b->engine); ++i) {
      assert(vm_advance(a, 0.22, &clock)); /* Four scheduler steps per frame. */
      while (b->steps < a->steps) {
        size_t count;
        assert(vm_step(b));
        const uint32_t *ids = jc_engine_sounds(b->engine, &count);
        observe(&expected, ids, count, true);
      }
      assert(!memcmp(jc_engine_frame(a->engine)->px,
                     jc_engine_frame(b->engine)->px, WIDTH * HEIGHT));
      assert(observed.calls == expected.calls &&
             observed.events == expected.events &&
             observed.hash == expected.hash && observed.enabled);
    }
    total += observed.events;
    if (observed.events)
      printf("audio scene %d: %llu events\n", scene,
             (unsigned long long)observed.events);
    vm_shutdown(a);
    vm_shutdown(b);
  }
  assert(total > 0);
  cfg.sounds = false;
  assert(vm_init(a, dir, 4242, &clock, &cfg, 20));
  Events disabled = {0};
  a->sound_observer = observe;
  a->sound_user = &disabled;
  assert(vm_step(a) && disabled.calls == 1 && !disabled.enabled);
  vm_shutdown(a);
  free(a);
  free(b);
  printf("audio observer: 79 scenes, %llu events, catch-up without loss, "
         "canvas unchanged, config.sounds honored\n",
         (unsigned long long)total);
  return 0;
}
