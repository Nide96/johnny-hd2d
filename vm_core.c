#include "vm_core.h"
#include <stdio.h>
#include <string.h>

static void capture(void *user, const GfxCommand *command) {
  VmCore *vm = user;
  if (!vm->capture_started) {
    jc_render_frame_base(vm->scene.current, jc_engine_base(vm->engine)->px);
    vm->capture_started = true;
  }
  jc_render_frame_command(vm->scene.current, command);
}
bool vm_init(VmCore *vm, const char *dir, uint64_t seed, const JCClock *clock,
             const JCConfig *config, int forced_scene) {
  memset(vm, 0, sizeof(*vm));
  vm->clock = *clock;
  if (!jc_assets_load(&vm->assets, dir, &vm->error))
    return false;
  vm->engine = jc_engine_create(&vm->assets, config, seed, &vm->error);
  if (!vm->engine)
    return false;
  if (forced_scene >= 0 &&
      !jc_engine_force_scene(vm->engine, (size_t)forced_scene)) {
    snprintf(vm->error.message, sizeof(vm->error.message),
             "invalid forced scene: %d", forced_scene);
    return false;
  }
  if (!scene_init(&vm->scene)) {
    snprintf(vm->error.message, sizeof(vm->error.message),
             "render frame allocation failed");
    return false;
  }
  const Canvas *intro = jc_engine_frame(vm->engine);
  jc_render_frame_base(vm->scene.previous, intro->px);
  jc_render_frame_base(vm->scene.current, intro->px);
  jc_engine_set_trace(vm->engine, capture, vm);
  return true;
}
bool vm_step(VmCore *vm) {
  vm->step_ms += JC_STEP_MS;
  while (vm->tick_ms + JC_TICK_MS <= vm->step_ms) {
    jc_engine_tick(vm->engine);
    vm->tick_ms += JC_TICK_MS;
  }
  scene_swap(&vm->scene);
  vm->capture_started = false;
  const uint64_t before = jc_engine_scenes_done(vm->engine);
  const uint64_t epoch = jc_engine_epoch(vm->engine);
  if (!jc_engine_step(vm->engine, &vm->clock, &vm->error))
    return false;
  if (!vm->capture_started || vm->scene.current->overflow) {
    if (vm->scene.current->overflow)
      ++vm->flattened_frames;
    jc_render_frame_base(vm->scene.current, jc_engine_frame(vm->engine)->px);
    vm->scene.current->modern_compatible = false;
  }
  JCWorldView view = jc_engine_world_view(vm->engine);
  vm->scene.current->origin_x = view.origin_x;
  vm->scene.current->origin_y = view.origin_y;
  vm->scene.current->modern_allowed = view.modern_allowed;
  vm->scene.current->walk_preview = view.walk_preview;
  vm->scene.current->scene_id = view.scene_id;
  vm->scene.current->cut_id =
      jc_engine_epoch(vm->engine) * UINT64_C(0x9e3779b97f4a7c15) ^ before;
  vm->scene.current->serial = ++vm->steps;
  const bool cut = before != jc_engine_scenes_done(vm->engine) ||
                   epoch != jc_engine_epoch(vm->engine);
  vm->scene.interpolate = !cut && !vm->previous_cut;
  vm->previous_cut = cut;
  if (vm->sound_observer) {
    size_t count;
    const uint32_t *ids = jc_engine_sounds(vm->engine, &count);
    vm->sound_observer(vm->sound_user, ids, count,
                       jc_engine_config(vm->engine)->sounds);
  }
  return true;
}
bool vm_advance(VmCore *vm, double elapsed, const JCClock *clock) {
  vm->clock = *clock;
  if (!(elapsed >= 0.0))
    elapsed = 0.0;
  if (elapsed > MAX_FRAME_SECONDS) {
    vm->scene.dropped_seconds += elapsed - MAX_FRAME_SECONDS;
    elapsed = MAX_FRAME_SECONDS;
  }
  vm->scene.visual_seconds += elapsed;
  vm->scene.accumulator += elapsed;
  while (vm->scene.accumulator >= VM_STEP_SECONDS) {
    if (!vm_step(vm))
      return false;
    vm->scene.accumulator -= VM_STEP_SECONDS;
  }
  return true;
}
bool vm_verify_frame(VmCore *vm, uint8_t *scratch) {
  jc_render_frame_raster(vm->scene.current, scratch);
  const uint8_t *expected = jc_engine_frame(vm->engine)->px;
  for (size_t i = 0; i < WIDTH * HEIGHT; ++i)
    if (scratch[i] != expected[i]) {
      snprintf(
          vm->error.message, sizeof(vm->error.message),
          "GPU command capture differs at step %llu pixel (%zu,%zu): %u != %u",
          (unsigned long long)vm->steps, i % WIDTH, i / WIDTH, scratch[i],
          expected[i]);
      return false;
    }
  return true;
}
void vm_shutdown(VmCore *vm) {
  jc_engine_destroy(vm->engine);
  vm->engine = NULL;
  jc_assets_free(&vm->assets);
  scene_free(&vm->scene);
}
