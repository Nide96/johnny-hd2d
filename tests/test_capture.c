/* Exact draw-stream reconstruction across every playable original scene. */
#include "vm_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv) {
  JCClock c = {1993, 6, 1, 10, 0};
  VmCore *vm = calloc(1, sizeof *vm);
  uint8_t *scratch = malloc(WIDTH * HEIGHT);
  unsigned tested = 0;
  uint64_t frames = 0;
  for (int scene = 1; scene < 80; scene++) {
    JCConfig cfg = jc_config_default();
    cfg.num_days = cfg.intro = 1;
    if (!vm_init(vm, (argc > 1 ? argv[1] : "assets/original"), 4242, &c, &cfg,
                 scene)) {
      fprintf(stderr, "init scene %d: %s\n", scene, vm->error.message);
      return 1;
    }
    unsigned step;
    bool previous_cut = false;
    for (step = 0; step < 6000; step++) {
      uint64_t epoch = jc_engine_epoch(vm->engine),
               done = jc_engine_scenes_done(vm->engine);
      if (!vm_step(vm) || !vm_verify_frame(vm, scratch)) {
        fprintf(stderr, "scene %d step %u: %s\n", scene, step,
                vm->error.message);
        return 2;
      }
      frames++;
      assert(vm->scene.current->scene_id == (unsigned)scene);
      bool cut = epoch != jc_engine_epoch(vm->engine) ||
                 done != jc_engine_scenes_done(vm->engine);
      if (cut || previous_cut)
        assert(!vm->scene.interpolate);
      previous_cut = cut;
      if (jc_engine_scenes_done(vm->engine))
        break;
    }
    if (step == 6000) {
      fprintf(stderr, "scene %d stalled\n", scene);
      return 3;
    }
    if (vm->flattened_frames) {
      fprintf(stderr, "unexpected atlas fallback\n");
      return 4;
    }
    tested++;
    vm_shutdown(vm);
  }
  printf("capture scenes: %u forced scenes, %llu frames capture exact\n",
         tested, (unsigned long long)frames);
  JCConfig cfg = jc_config_default();
  assert(
      vm_init(vm, (argc > 1 ? argv[1] : "assets/original"), 1, &c, &cfg, -1));
  for (unsigned i = 0; i < 144; ++i)
    assert(vm_advance(vm, 1.0 / 144.0, &c));
  assert(vm->steps == 18 && jc_engine_now(vm->engine) == 61);
  assert(fabs(vm->scene.accumulator - 0.01) < 1e-9);
  assert(vm_advance(vm, 5.0, &c));
  assert(vm->steps == 22 && fabs(vm->scene.dropped_seconds - 4.75) < 1e-9);
  vm_shutdown(vm);
  puts("capture timing: 144 render callbacks, 55/16ms clocks, stall cap and "
       "scene cuts OK");
  c = (JCClock){2026, 10, 18, 23, 30};
  cfg = jc_config_default();
  assert(
      vm_init(vm, (argc > 1 ? argv[1] : "assets/original"), 1, &c, &cfg, -1));
  assert(vm_step(vm));
  assert(!vm->scene.current->modern_allowed);
  vm_shutdown(vm);
  puts("modern presentation: original night phase falls back to classic");
  free(vm);
  free(scratch);
  return 0;
}
