#include "hybrid.h"
#include "vm_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static void lighting(void) {
  JCDraw hero = {0};
  strcpy(hero.resource, "JOHNWALK.BMP");
  assert(jc_hybrid_johnny(&hero));
  strcpy(hero.resource, "SLEEP.BMP");
  hero.source_sprite = 12;
  assert(jc_hybrid_johnny(&hero));
  strcpy(hero.resource, "MJREAD.BMP");
  hero.source_sprite = 31;
  assert(!jc_hybrid_johnny(&hero));
  strcpy(hero.resource, "SSUZY1.BMP");
  assert(!jc_hybrid_johnny(&hero));
  strcpy(hero.resource, "TRUNK.BMP");
  assert(!jc_hybrid_johnny(&hero));

  JCDraw fragment = {.domain = GFX_ACTOR,
                     .w = 16,
                     .h = 16,
                     .transparent = true,
                     .source_sprite = 12};
  strcpy(fragment.resource, "SLEEP.BMP");
  assert(!jc_hybrid_shadow(&fragment, 630, 480, 16, 16));
  fragment.source_sprite = 0;
  assert(jc_hybrid_shadow(&fragment, 630, 480, 16, 16));
  JCHybridLight noon = jc_hybrid_light(12), night = jc_hybrid_light(0),
                wrap = jc_hybrid_light(24);
  assert(noon.sun[2] > night.sun[2] && noon.tint[0] > night.tint[0]);
  assert(jc_hybrid_light(8).skew * jc_hybrid_light(17).skew < 0);
  for (int i = 0; i < 4; ++i)
    assert(fabsf(night.tint[i] - wrap.tint[i]) < 1e-5f);
  for (int n = 1; n <= 1440; ++n) {
    JCHybridLight a = jc_hybrid_light((n - 1) / 60.0),
                  b = jc_hybrid_light(n / 60.0);
    for (int i = 0; i < 4; ++i)
      assert(fabsf(a.tint[i] - b.tint[i]) < .02f);
  }
  assert(jc_hybrid_ground(645, 505) && !jc_hybrid_ground(645, 700) &&
         !jc_hybrid_ground(100, 505));
  uint8_t red[3] = {255, 0, 0}, mapped[4];
  jc_hybrid_color(red, mapped);
  assert(mapped[0] > mapped[1] && mapped[1] > mapped[2] && mapped[1] > 50 &&
         mapped[3] == 255);
}
int main(int argc, char **argv) {
  lighting();
  const char *dir = argc > 1 ? argv[1] : "assets/original";
  JCAssets assets = {0};
  JCError error = {{0}};
  assert(jc_assets_load(&assets, dir, &error));
  size_t sheets = 0, sprites = 0, hd = 0;
  for (size_t i = 0; i < assets.archive.count; ++i) {
    const char *name = assets.archive.files[i].name;
    size_t n = strlen(name);
    if (n < 4 || strcmp(name + n - 4, ".BMP"))
      continue;
    JCBmp bmp = {0};
    assert(jc_archive_bmp(&assets.archive, name, &bmp, &error));
    ++sheets;
    sprites += bmp.count;
    for (size_t id = 0; id < bmp.count; ++id) {
      JCDraw d = {.source_sprite = (uint16_t)id};
      snprintf(d.resource, sizeof d.resource, "%s", name);
      JCHDPose p = jc_hybrid_pose(&d);
      if (p.sheet >= 0) {
        assert(p.sheet < JC_HD_FISH);
        ++hd;
      }
      if (!strncmp(name, "MJFISH", 6))
        assert(p.sheet == JC_HD_NONE);
    }
    jc_bmp_free(&bmp);
  }
  assert(hd == 84);
  /* The GPU copy removes only audited grey pixels. The VM/source stays exact.
   */
  JCRenderFrame frame;
  assert(jc_render_frame_init(&frame));
  uint8_t *copy = malloc((size_t)JC_ATLAS_W * JC_ATLAS_H);
  assert(copy);
  unsigned masks = 0, removed = 0, coconut_masks = 0;
  const char *families[] = {"JOHNWALK.BMP", "MJ_AMB.BMP",  "SLEEP.BMP",
                            "MJREAD.BMP",   "MJFISH1.BMP", "MJFISH2.BMP", "MJCOCO.BMP"};
  for (size_t n = 0; n < sizeof families / sizeof *families; ++n) {
    JCBmp bmp = {0};
    assert(jc_archive_bmp(&assets.archive, families[n], &bmp, &error));
    for (size_t id = 0; id < bmp.count; ++id) {
      JCDraw d = {.w = bmp.sprites[id].w,
                  .h = bmp.sprites[id].h,
                  .source_sprite = (uint16_t)id};
      snprintf(d.resource, sizeof d.resource, "%s", families[n]);
      for (int y = 0; y < d.h; ++y)
        memcpy(frame.atlas + (size_t)y * JC_ATLAS_W,
               bmp.sprites[id].px + (size_t)y * d.w, (size_t)d.w);
      memcpy(copy, frame.atlas, (size_t)JC_ATLAS_W * JC_ATLAS_H);
      jc_sprite_strip_shadow(copy, &d);
      const JCShadowMask *m = jc_sprite_shadow_mask(&d);
      if (m) {
        ++masks;
        if (n == 6) ++coconut_masks;
      }
      for (int y = 0; y < d.h; ++y)
        for (int x = 0; x < d.w; ++x) {
          size_t at = (size_t)y * JC_ATLAS_W + x;
          if (copy[at] != frame.atlas[at]) {
            assert(frame.atlas[at] == 8 && copy[at] == 0 &&
                   jc_sprite_shadow_pixel(m, x, y));
            ++removed;
          }
        }
      /* Original grey newspaper and hat shading must survive. */
      if (n == 3 && id == 3)
        assert(frame.atlas[37u * JC_ATLAS_W + 29] == 8 &&
               copy[37u * JC_ATLAS_W + 29] == 8);
      if (n == 2 && id == 4)
        assert(frame.atlas[4u * JC_ATLAS_W + 11] == 8 &&
               copy[4u * JC_ATLAS_W + 11] == 8);
      if (n == 2 && id != 12)
        assert(m);
      if (n == 6) {
        bool body = id <= 6 || (id >= 8 && id <= 15) ||
                    (id >= 20 && id <= 23) || id == 31 || id == 32;
        assert((m != NULL) == body);
        if (id == 1) {
          /* Grey cap shading is not the ground shadow (reported pose). */
          assert(frame.atlas[4u * JC_ATLAS_W + 70] == 8);
          assert(copy[4u * JC_ATLAS_W + 70] == 8);
          assert(!jc_sprite_unhandled_shadow(&frame, &d));
        }
      }
      JCVisibleBounds b = jc_sprite_body_bounds(&frame, &d);
      assert(b.bottom > 0 && b.bottom <= d.h && b.right <= d.w);
      if (n == 2)
        assert(jc_hybrid_contact(&d, b));
      if (n == 4)
        assert(!jc_hybrid_contact(&d, b));
    }
    jc_bmp_free(&bmp);
  }
  assert(masks > 100 && removed > 10000 && coconut_masks == 21);
  free(copy);
  JCDraw unknown = {.w = 16, .h = 16};
  strcpy(unknown.resource, "UNREVIEWED.BMP");
  memset(frame.atlas, 0, (size_t)JC_ATLAS_W * JC_ATLAS_H);
  for (int y = 12; y < 16; ++y)
    memset(frame.atlas + (size_t)y * JC_ATLAS_W, 8, 16);
  assert(jc_sprite_unhandled_shadow(&frame, &unknown));
  unknown.resource[0] = 0;
  assert(!jc_sprite_unhandled_shadow(&frame, &unknown));
  jc_render_frame_free(&frame);
  printf("Audited shadow masks: %u sprites, %u grey pixels removed from GPU "
         "copies only\n",
         masks, removed);
  jc_assets_free(&assets);
  VmCore *v = calloc(1, sizeof *v);
  uint8_t *scratch = malloc(WIDTH * HEIGHT);
  assert(v && scratch);
  JCClock clock = {1993, 6, 1, 17, 0};
  JCConfig cfg = jc_config_default();
  cfg.num_days = 6;
  cfg.intro = 1;
  assert(vm_init(v, dir, 4242, &clock, &cfg, 51));
  unsigned rafts = 0;
  for (unsigned n = 0; n < 120; ++n) {
    assert(vm_step(v) && vm_verify_frame(v, scratch));
    const JCRenderFrame *f = v->scene.current;
    assert(!jc_hybrid_hd_sequence(f));
    for (size_t i = 0; i < f->count; ++i)
      if (f->draws[i].domain == GFX_RAFT) {
        bool found = false;
        for (size_t j = 0; j < f->actor_count; ++j)
          if (f->actors[j].domain == GFX_RAFT &&
              f->actors[j].ax == f->draws[i].ax &&
              f->actors[j].ay == f->draws[i].ay)
            found = true;
        assert(found);
        ++rafts;
        float bottom = jc_hybrid_bottom(f, &f->draws[i]);
        assert(bottom > 0 && bottom <= 1);
      }
  }
  assert(rafts > 0);
  vm_shutdown(v);
  assert(vm_init(v, dir, 4242, &clock, &cfg, -1));
  jc_engine_walk_preview(v->engine);
  for (unsigned n = 0; n < 600; ++n) {
    assert(vm_step(v) && vm_verify_frame(v, scratch));
    assert(!jc_hybrid_hd_sequence(v->scene.current));
  }
  /* A partial clip or a composite body rejects the entire HD sequence. */
  JCRenderFrame *f = v->scene.current;
  size_t walker = 0;
  while (walker < f->actor_count && !f->actors[walker].walk)
    ++walker;
  assert(walker < f->actor_count);
  f->actors[walker].clip.x1 = 1;
  assert(!jc_hybrid_hd_sequence(f));
  f->actors[walker].clip = CLIP_FULL;
  f->actors[f->actor_count++] = (JCDraw){.domain = GFX_ACTOR};
  assert(!jc_hybrid_hd_sequence(f));
  vm_shutdown(v);
  free(scratch);
  free(v);
  printf("Hybrid: %zu original BMP sheets, %zu sprites, 84 retained HD "
         "mappings; runtime R8 fallback for the rest; continuous light, ground "
         "exclusion, raft retention and exact classic canvas OK\n",
         sheets, sprites);
}
