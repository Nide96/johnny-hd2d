/* Integration checks against user's original assets; no GPU/audio needed. */
#include "engine.h"
#include "rng.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void drive(JCEngine *e, const JCClock *c, uint64_t *ticks,
                  uint64_t step) {
  JCError err = {{0}};
  while ((*ticks + 1) * JC_TICK_MS <= step * JC_STEP_MS) {
    ++*ticks;
    jc_engine_tick(e);
  }
  if (!jc_engine_step(e, c, &err)) {
    fprintf(stderr, "engine step %llu: %s\n", (unsigned long long)step,
            err.message);
    abort();
  }
}
static unsigned traces;
static void trace(void *ctx, const GfxCommand *q) {
  const Canvas *c = ctx;
  assert(q->result == c->px);
  ++traces;
}
int main(int argc, char **argv) {
  JCAssets data = {0};
  JCError err = {{0}};
  JCClock c = {1993, 6, 1, 10, 0};
  JCEngine *a, *b;
  uint64_t ticks_a = 0, ticks_b = 0, step;
  size_t i;
  Canvas *can = calloc(1, sizeof *can);
  uint16_t table[56] = {0};
  Rng rng;
  assert(can);
  table[24] = 1;
  table[55] = 65535;
  rng_init(&rng, table, 24, 55);
  assert(rng_next(&rng) == 0);
  can->trace = trace;
  can->trace_user = can;
  canvas_fill(can, 7);
  assert(traces == 1 && can->px[0] == 7);
  free(can);
  if (!jc_assets_load(&data, argc > 1 ? argv[1] : "assets/original", &err)) {
    fprintf(stderr, "%s\n", err.message);
    return 1;
  }
  a = jc_engine_create(&data, NULL, 4242, &err);
  b = jc_engine_create(&data, NULL, 4242, &err);
  assert(a && b);
  for (step = 1; step <= 6000; ++step) {
    drive(a, &c, &ticks_a, step);
    drive(b, &c, &ticks_b, step);
    assert(!memcmp(jc_engine_frame(a)->px, jc_engine_frame(b)->px,
                   WIDTH * HEIGHT));
  }
  assert(jc_engine_scenes_done(a) >= 3);
  printf("6000 deterministic steps: %llu scenes, %u ticks\n",
         (unsigned long long)jc_engine_scenes_done(a), jc_engine_now(a));
  jc_engine_destroy(a);
  jc_engine_destroy(b);
  for (i = 0; i < data.exe.scene_count; ++i) {
    uint64_t ticks = 0;
    size_t n;
    JCConfig cfg = jc_config_default();
    if (!data.exe.scenes[i].ads_tag || !data.exe.scenes[i].ads_id)
      continue;
    cfg.num_days = cfg.intro =
        data.exe.scenes[i].day ? data.exe.scenes[i].day : 1;
    a = jc_engine_create(&data, &cfg, 4242, &err);
    assert(a && jc_engine_force_scene(a, i));
    for (step = 1; step <= 6000 && !jc_engine_scenes_done(a); ++step)
      drive(a, &c, &ticks, step);
    if (!jc_engine_scenes_done(a)) {
      fprintf(stderr, "scene %zu did not complete\n", i);
      return 1;
    }
    (void)jc_engine_sounds(a, &n);
    jc_engine_destroy(a);
  }
  printf("all playable catalog scenes completed\n");
  jc_assets_free(&data);
  return 0;
}
