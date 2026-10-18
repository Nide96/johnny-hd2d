#include "../ads.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned sounds;
static void sound(void *unused, uint32_t id) {
  (void)unused;
  assert(id == 7);
  sounds++;
}
static void semantics(void) {
  TtmScene scene;
  TtmCtx ctx = {0};
  TtmFrameResult result;
  VmEnv env = {0};
  Rng rng = {0}, after;
  uint16_t table[56] = {1};
  Canvas *canvas = calloc(1, sizeof *canvas);
  JCOp ops[] = {{0x1021, {4, 0, 0, 0, 0, 0}, NULL},
                {0x2022, {3, 15, 0, 0, 0, 0}, NULL},
                {0xC051, {7, 0, 0, 0, 0, 0}, NULL},
                {0x0FF0, {0}, NULL}};
  JCFrame frame = {ops, 4};
  assert(canvas);
  rng_init(&rng, table, 54, 23);
  ctx.ttm.frames = &frame;
  ctx.ttm.frame_count = 1;
  ttm_scene_init(&scene, 0, 2, 3, 0);
  env.work = canvas;
  env.bg = canvas;
  env.front = canvas;
  env.rng = &rng;
  env.sound = sound;
  assert(ttm_run_frame(&ctx, &scene, &env, &result));
  assert(result.has_delay && result.delay >= 3 && result.delay < 15 &&
         sounds == 1);
  after = rng;
  scene.redraw = true;
  assert(ttm_run_frame(&ctx, &scene, &env, &result));
  assert(result.has_delay && result.delay == 4 && sounds == 1 &&
         !memcmp(&after, &rng, sizeof rng));
  scene.delay = 4;
  scene.next_time = 10;
  scene.jump = 9;
  assert(!ttm_scene_advance(&scene, 9) && scene.cur == 0 && scene.redraw);
  assert(ttm_scene_advance(&scene, 10) && scene.cur == 9 && !scene.redraw &&
         scene.next_time == 14);
  scene.plays = 65535;
  scene.end_time = 123;
  ttm_scene_reset(&scene);
  assert(scene.plays == 0 && scene.state == TTM_IDLE && scene.cur == 0);
  ops[0].code = 0xDEAD;
  scene.redraw = false;
  assert(!ttm_run_frame(&ctx, &scene, &env, &result) && env.failed &&
         strstr(env.error, "dead"));
  free(canvas);
}

/* A small in-memory archive tests malformed scheduler scripts without fixtures.
 */
static bool synthetic_ads(AdsRuntime *ads, const uint16_t *words,
                          size_t count) {
  uint8_t data[256] = {'R', 'E', 'S', ':', 2, 0, 0, 0, 0, 0, 'S', 'C',
                       'R', ':', 0,   0,   0, 0, 1, 0, 0, 0, 0,   0};
  JCResource file = {"TEST.ADS", data, 0};
  JCArchive archive = {0};
  size_t i, bytes = count * 2;
  assert(bytes < 128);
  data[14] = (uint8_t)(bytes + 6);
  data[19] = (uint8_t)bytes;
  data[23] = (uint8_t)bytes;
  for (i = 0; i < count; ++i) {
    data[24 + i * 2] = (uint8_t)words[i];
    data[25 + i * 2] = (uint8_t)(words[i] >> 8);
  }
  file.size = bytes + 24;
  archive.files = &file;
  archive.count = 1;
  return ads_load(ads, &archive, 1, "TEST.ADS");
}
static void malformed_scripts(void) {
  const uint16_t truncated[] = {1, 0x2000, 0xFFFF};
  const uint16_t unknown[] = {1, 0xDEAD, 0xFFFF};
  const uint16_t bad_random[] = {1, 0x3010, 0x3020, 1, 0xFFFF};
  AdsRuntime ads = {0};
  VmEnv env = {0};
  Rng rng = {0};
  env.rng = &rng;
  assert(!synthetic_ads(&ads, truncated, 3) && ads.failed);
  ads_free(&ads);
  assert(synthetic_ads(&ads, unknown, 3));
  ads_start_tag(&ads, 1);
  (void)ads_update(&ads, &env);
  assert(ads.failed && strstr(ads.error, "dead"));
  ads_free(&ads);
  assert(synthetic_ads(&ads, bad_random, 5));
  ads_start_tag(&ads, 1);
  (void)ads_update(&ads, &env);
  assert(ads.failed && strstr(ads.error, "RANDOM"));
  ads_free(&ads);
}

int main(int argc, char **argv) {
  JCAssets assets = {0};
  JCError error = {{0}};
  JCBmp john = {0};
  Canvas *work = calloc(1, sizeof *work), *bg = calloc(1, sizeof *bg),
         *front = calloc(1, sizeof *front);
  size_t i, done = 0, script_count = 0;
  semantics();
  malformed_scripts();
  assert(work && bg && front);
  assert(
      jc_assets_load(&assets, argc > 1 ? argv[1] : "assets/original", &error));
  assert(jc_archive_bmp(&assets.archive, "JOHNWALK.BMP", &john, &error));
  for (i = 0; i < 256; i++)
    if (assets.exe.ads_names[i]) {
      AdsRuntime ads = {0};
      if (!ads_load(&ads, &assets.archive, (uint8_t)i,
                    assets.exe.ads_names[i])) {
        fprintf(stderr, "%s: %s\n", assets.exe.ads_names[i], ads.error);
        return 1;
      }
      assert(ads.tag_count > 0);
      script_count++;
      ads_free(&ads);
    }
  for (i = 0; i < assets.exe.scene_count; i++) {
    const JCSceneDef *def = &assets.exe.scenes[i];
    AdsRuntime ads = {0};
    VmEnv env = {0};
    Rng rng;
    unsigned step;
    if (def->ads_id == 255)
      continue;
    if (i == 0 && def->ads_id == 0 && def->ads_tag == 0)
      continue;
    assert(assets.exe.ads_names[def->ads_id]);
    assert(ads_load(&ads, &assets.archive, def->ads_id,
                    assets.exe.ads_names[def->ads_id]));
    rng_init(&rng, assets.exe.rng_table, assets.exe.rng_j, assets.exe.rng_k);
    rng_warm_up(&rng, 4242);
    env.work = work;
    env.bg = bg;
    env.front = front;
    env.rng = &rng;
    env.archive = &assets.archive;
    env.johnwalk = &john;
    ads_start_tag(&ads, def->ads_tag);
    for (step = 0; step < 30000; step++) {
      env.now = (uint32_t)(((uint64_t)step * 55u) / 16u);
      (void)ads_update(&ads, &env);
      if (ads.failed) {
        fprintf(stderr, "scene %zu %s:%u step %u: %s\n", i,
                assets.exe.ads_names[def->ads_id], def->ads_tag, step,
                ads.error);
        return 1;
      }
      if (!ads_tag_busy(&ads, def->ads_tag))
        break;
    }
    if (step == 30000) {
      fprintf(stderr, "scene %zu stalled\n", i);
      return 1;
    }
    done++;
    ads_free(&ads);
  }
  printf("VM tests: %zu ADS loaded, %zu original catalog scenes completed; "
         "redraw/delay/error semantics OK\n",
         script_count, done);
  jc_bmp_free(&john);
  jc_assets_free(&assets);
  free(work);
  free(bg);
  free(front);
  return 0;
}
