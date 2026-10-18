/* Reproducible presentation coverage from actual ADS/TTM execution.
 * No original pixels or proprietary script bytes are exported. */
#include "vm_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
  char name[14];
  unsigned id, draws;
} Use;
static Use uses[4096];
int main(int argc, char **argv) {
  VmCore *v = calloc(1, sizeof *v);
  if (!v)
    return 1;
  puts("{\"seed\":4242,\"clock\":\"1993-06-01T10:00\",\"scenes\":[");
  for (int scene = 1; scene < 80; ++scene) {
    JCConfig cfg = jc_config_default();
    JCClock clock = {1993, 6, 1, 10, 0};
    cfg.num_days = cfg.intro = 1;
    unsigned count = 0, steps = 0, eligible = 0, compatible = 0;
    if (!vm_init(v, argc > 1 ? argv[1] : "assets/original", 4242, &clock, &cfg,
                 scene))
      goto fail;
    do {
      if (++steps > 6000 || !vm_step(v))
        goto fail;
      const JCRenderFrame *f = v->scene.current;
      eligible += f->modern_allowed;
      compatible += f->modern_allowed && f->modern_compatible;
      for (size_t i = 0; i < f->actor_count; ++i) {
        const JCDraw *d = &f->actors[i];
        if (!d->resource[0])
          continue;
        unsigned j;
        for (j = 0; j < count; ++j)
          if (uses[j].id == d->source_sprite &&
              !strcmp(uses[j].name, d->resource))
            break;
        if (j == count) {
          if (count == 4096)
            goto fail;
          memcpy(uses[j].name, d->resource, sizeof uses[j].name);
          uses[j].id = d->source_sprite;
          uses[j].draws = 0;
          count++;
        }
        uses[j].draws++;
      }
    } while (!jc_engine_scenes_done(v->engine));
    printf("%s{\"scene\":%d,\"steps\":%u,\"modern_allowed_steps\":%u,"
           "\"compatible_steps\":%u,\"sprites\":[",
           scene == 1 ? "" : ",\n", scene, steps, eligible, compatible);
    for (unsigned j = 0; j < count; ++j)
      printf("%s{\"resource\":\"%s\",\"id\":%u,\"draws\":%u}", j ? "," : "",
             uses[j].name, uses[j].id, uses[j].draws);
    printf("]}");
    vm_shutdown(v);
  }
  puts("\n]}");
  free(v);
  return 0;
fail:
  fprintf(stderr, "Scene inventory failed: %s\n", v->error.message);
  vm_shutdown(v);
  free(v);
  return 1;
}
