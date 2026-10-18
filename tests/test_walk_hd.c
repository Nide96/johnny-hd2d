/* HD is presentation-only: the walk VM, footprint and indexed raster survive.
 */
#include "vm_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
  VmCore *v = calloc(1, sizeof *v);
  uint8_t *pixels = malloc(WIDTH * HEIGHT);
  JCClock clock = {2026, 10, 18, 12, 0};
  JCConfig cfg = jc_config_default();
  unsigned poses = 0, directions = 0, walks = 0, occlusions = 0;
  assert(v && pixels);
  assert(
      vm_init(v, argc > 1 ? argv[1] : "assets/original", 1, &clock, &cfg, -1));
  jc_engine_walk_preview(v->engine);
  for (unsigned n = 0; n < 600; ++n) {
    assert(vm_step(v) && vm_verify_frame(v, pixels));
    JCRenderFrame *f = v->scene.current;
    assert(f->walk_preview && f->modern_allowed && f->modern_compatible);
    assert(f->actor_count >= 1 && f->actor_count <= 3 && !f->overflow);
    assert(f->scenery_count == 3);
    assert(f->scenery[1].source_sprite == 13);
    assert(f->scenery[1].x - f->origin_x == 443 &&
           f->scenery[1].y - f->origin_y == 148);
    assert(f->scenery[1].w == 24 && f->scenery[1].h == 145);
    const JCDraw *d = NULL;
    for (size_t i = 0; i < f->actor_count; ++i) {
      if (f->actors[i].domain == GFX_OCCLUDER)
        ++occlusions;
      else if (f->actors[i].walk) {
        assert(!d);
        d = &f->actors[i];
      } else
        assert(f->actors[i].domain == GFX_RAFT);
    }
    assert(d);
    assert(d->walk && d->source_sprite >= 1 && d->source_sprite <= 8);
    poses |= 1u << ((d->source_sprite + 4u) % 8u);
    directions |= (d->flags & FLIP_H) ? 2u : 1u;
    assert(d->x - f->origin_x >= 380 && d->x - f->origin_x <= 480);
    ++walks;
    assert(jc_engine_scenes_done(v->engine) == 0);
  }
  assert(poses == 255 && directions == 3 && occlusions > 0);
  assert(jc_engine_config(v->engine)->num_days == 1);
  vm_shutdown(v);
  /* A normal story starts inside an action. Its first side-walk occurs later.
   */
  assert(
      vm_init(v, argc > 1 ? argv[1] : "assets/original", 1, &clock, &cfg, -1));
  bool story_walk = false;
  for (unsigned n = 0; n < 1000; ++n) {
    assert(vm_step(v) && vm_verify_frame(v, pixels));
    const JCRenderFrame *f = v->scene.current;
    for (size_t i = 0; i < f->actor_count; ++i) {
      const JCDraw *d = &f->actors[i];
      if (f->modern_allowed && d->walk && d->source_sprite >= 1 &&
          d->source_sprite <= 8)
        story_walk = true;
    }
  }
  assert(story_walk);
  vm_shutdown(v);
  free(v);
  free(pixels);
  printf("HD walk: %u frames, 8 poses, 2 directions, unchanged indexed raster, "
         "no story progression\n",
         walks);
}
