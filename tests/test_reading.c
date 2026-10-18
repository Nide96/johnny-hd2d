/* Semantic scene replay plus explicit multipart substitution contracts. */
#include "character_assets.h"
#include "vm_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void geometry(const JCAssets *assets) {
  JCBmp bmp = {0};
  JCError error = {{0}};
  assert(jc_archive_bmp(&assets->archive, "MJREAD.BMP", &bmp, &error));
  assert(bmp.count == 36);
  JCDraw d = {0};
  strcpy(d.resource, "MJREAD.BMP");
  for (unsigned id = 0; id < 36; id++) {
    d.source_sprite = (uint16_t)id;
    JCHDPose pose = jc_character_pose(&d);
    assert(pose.sheet == (id < 24 ? JC_HD_READ : JC_HD_READ_PROPS));
    assert(pose.cell == (int)(id < 24 ? id : id - 24) && pose.fit_rect);
    bool body = id <= 18 || (id >= 21 && id <= 23) || (id >= 26 && id <= 30);
    assert(pose.shadow == body);
    const JCSprite *s = &bmp.sprites[id];
    assert(reading_bounds[id].width == s->w &&
           reading_bounds[id].height == s->h);
    for (unsigned flags = 0; flags < 4; flags++) {
      JCHDRect r = jc_reading_footprint(
          id, (uint8_t)flags, (JCHDRect){100, 200, s->w * 1.8f, s->h * 1.8f});
      int xmin = s->w, ymin = s->h, xmax = -1, ymax = -1;
      /* Derive the reference footprint from decoded pixels, including mirror,
       * independently of generated bounds or runtime geometry arithmetic. */
      for (int y = 0; y < s->h; y++)
        for (int x = 0; x < s->w; x++) {
          int xx = (flags & FLIP_H) ? s->w - 1 - x : x;
          int yy = (flags & FLIP_V) ? s->h - 1 - y : y;
          if (s->px[yy * s->w + xx]) {
            if (x < xmin)
              xmin = x;
            if (x > xmax)
              xmax = x;
            if (y < ymin)
              ymin = y;
            if (y > ymax)
              ymax = y;
          }
        }
      assert(fabsf(r.x - (100 + xmin * 1.8f)) < .001f);
      assert(fabsf(r.y - (200 + ymin * 1.8f)) < .001f);
      assert(fabsf(r.w - (xmax - xmin + 1) * 1.8f) < .001f);
      assert(fabsf(r.h - (ymax - ymin + 1) * 1.8f) < .001f);
    }
  }
  d.source_sprite = 36;
  assert(jc_character_pose(&d).sheet == JC_HD_NONE);
  jc_bmp_free(&bmp);
}
int main(int argc, char **argv) {
  static const int scenes[] = {6, 7, 10, 34, 35, 36, 38};
  VmCore *v = calloc(1, sizeof *v);
  uint8_t *scratch = malloc(WIDTH * HEIGHT);
  unsigned draws = 0, frames = 0, leaf_poses = 0;
  uint64_t seen = 0;
  assert(v && scratch);
  for (unsigned n = 0; n < sizeof scenes / sizeof scenes[0]; n++) {
    JCConfig cfg = jc_config_default();
    cfg.num_days = cfg.intro = 1;
    JCClock clock = {2026, 10, 18, 12, 0};
    assert(vm_init(v, argc > 1 ? argv[1] : "assets/original", 4242, &clock,
                   &cfg, scenes[n]));
    if (n == 0)
      geometry(&v->assets);
    unsigned steps = 0;
    do {
      assert(++steps < 6000 && vm_step(v) && vm_verify_frame(v, scratch));
      frames++;
      const JCRenderFrame *f = v->scene.current;
      for (size_t i = 0; i < f->actor_count; i++) {
        const JCDraw *d = &f->actors[i];
        if (scenes[n] == 35 && !strcmp(d->resource, "MJREAD.BMP") &&
            d->source_sprite == 20) {
          assert(d->w == 48 && d->h == 52);
          ++leaf_poses;
        }
        if (!strcmp(d->resource, "MJREAD.BMP")) {
          assert(d->source_sprite < 36);
          JCHDPose p = jc_character_pose(d);
          assert(p.sheet == JC_HD_READ || p.sheet == JC_HD_READ_PROPS);
          seen |= UINT64_C(1) << d->source_sprite;
          draws++;
        }
        /* Every Johnny body in the complete reading scene 34 is now covered,
         * including its oblique entry/exit, without replacing the lightbulb. */
        if (scenes[n] == 34 && !strcmp(d->resource, "JOHNWALK.BMP"))
          assert(jc_character_pose(d).sheet != JC_HD_NONE);
      }
    } while (!jc_engine_scenes_done(v->engine));
    assert(!v->flattened_frames);
    vm_shutdown(v);
  }
  assert((seen & (UINT64_C(1) << 24)) && (seen & (UINT64_C(1) << 25)));
  assert(leaf_poses > 0);
  for (unsigned id = 31; id <= 35; id++)
    assert(seen & (UINT64_C(1) << id));
  unsigned ids = 0;
  for (unsigned i = 0; i < 36; i++)
    ids += (seen >> i) & 1u;
  printf("Reading: 7 original scene replays, %u frames, %u MJREAD draws, %u "
         "distinct IDs; all 36 mappings + reflected decoded-pixel footprints, "
         "fragments/props without shadows, scene34 bodies covered, classic "
         "pixels exact\n",
         frames, draws, ids);
  free(scratch);
  free(v);
  return 0;
}
