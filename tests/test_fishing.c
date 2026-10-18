#include "character_assets.h"
#include "vm_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static void geometry(const JCAssets *assets, const char *name, unsigned count) {
  JCBmp bmp = {0};
  JCError error = {{0}};
  JCDraw d = {0};
  assert(jc_archive_bmp(&assets->archive, name, &bmp, &error));
  assert(bmp.count == count);
  strcpy(d.resource, name);
  for (unsigned id = 0; id < count; ++id) {
    d.source_sprite = (uint16_t)id;
    JCHDPose p = jc_character_pose(&d);
    bool deferred=count==30 && (id==19 || id==28 || id==29);
    if(deferred) assert(p.sheet==JC_HD_NONE);
    else assert(p.sheet >= JC_HD_FISH && p.sheet < JC_HD_SHEET_COUNT && p.fit_rect && !p.mirror);
    bool body = count == 30 ? ((id >= 3 && id <= 13) || id == 16 || id == 17 ||
                               id == 20 || id == 21)
                            : (id == 3 || (id >= 6 && id <= 14) || id == 24);
    if(!deferred) assert(p.shadow == body);
    const JCSprite *s = &bmp.sprites[id];
    for (unsigned flags = 0; flags < 4; ++flags) {
      int xmin = s->w, ymin = s->h, xmax = -1, ymax = -1;
      for (int y = 0; y < s->h; ++y)
        for (int x = 0; x < s->w; ++x) {
          int xx = (flags & FLIP_H) ? s->w - 1 - x : x,
              yy = (flags & FLIP_V) ? s->h - 1 - y : y;
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
      JCHDRect r = jc_fishing_footprint(
          &d, (uint8_t)flags, (JCHDRect){23, 47, s->w * 1.8f, s->h * 1.8f});
      assert(fabsf(r.x - (23 + xmin * 1.8f)) < .001f);
      assert(fabsf(r.y - (47 + ymin * 1.8f)) < .001f);
      assert(fabsf(r.w - (xmax - xmin + 1) * 1.8f) < .001f);
      assert(fabsf(r.h - (ymax - ymin + 1) * 1.8f) < .001f);
    }
  }
  d.source_sprite = (uint16_t)count;
  assert(jc_character_pose(&d).sheet == JC_HD_NONE);
  jc_bmp_free(&bmp);
}
int main(int argc, char **argv) {
  const char *dir = argc > 1 ? argv[1] : "assets/original";
  JCAssets assets = {0};
  JCError error = {{0}};
  assert(jc_assets_load(&assets, dir, &error));
  geometry(&assets, "MJFISH1.BMP", 30);
  geometry(&assets, "MJFISH2.BMP", 27);
  jc_assets_free(&assets);
  unsigned scenes[] = {9, 20, 21, 24, 25, 27};
  bool seen[2][30] = {{false}};
  unsigned long draws = 0, frames = 0;
  VmCore *v = calloc(1, sizeof *v);
  uint8_t *raster = malloc(WIDTH * HEIGHT);
  assert(v && raster);
  for (unsigned n = 0; n < sizeof scenes / sizeof *scenes; ++n) {
    JCClock clock = {1993, 6, 1, 10, 0};
    JCConfig cfg = jc_config_default();
    cfg.num_days = cfg.intro = 1;
    assert(vm_init(v, dir, 4242, &clock, &cfg, (int)scenes[n]));
    unsigned step = 0;
    do {
      assert(++step < 6000 && vm_step(v));
      ++frames;
      const JCRenderFrame *f = v->scene.current;
      assert(vm_verify_frame(v, raster));
      for (size_t i = 0; i < f->actor_count; ++i) {
        const JCDraw *d = &f->actors[i];
        JCHDPose p = jc_character_pose(d);
        if (p.sheet >= JC_HD_FISH) {
          ++draws;
          seen[!strcmp(d->resource, "MJFISH2.BMP")][d->source_sprite] = true;
        }
      }
    } while (!jc_engine_scenes_done(v->engine));
    vm_shutdown(v);
  }
  unsigned unique = 0;
  for (unsigned n = 0; n < 2; ++n)
    for (unsigned i = 0; i < 30; ++i)
      unique += seen[n][i];
  assert(draws > 1000 && unique > 25);
  printf("Fishing: 54 archived mappings, 3 deferred fragments; reflected decoded-pixel footprints, "
         "multipart shadows; 6 scenes, %lu frames, %lu draws, %u IDs; classic "
         "pixels exact\n",
         frames, draws, unique);
  free(raster);
  free(v);
}
