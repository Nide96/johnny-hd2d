#ifndef JC_VM_CORE_H
#define JC_VM_CORE_H
#include "engine.h"
#include "scene.h"
typedef struct {
  JCAssets assets;
  JCEngine *engine;
  SceneState scene;
  JCClock clock;
  JCError error;
  uint64_t step_ms, tick_ms, steps, flattened_frames;
  bool capture_started, previous_cut;
  /* Presentation observer, invoked once per successful step (including
   * catch-up steps). NULL in headless/tests; never changes scheduler state. */
  void (*sound_observer)(void *, const uint32_t *, size_t, bool);
  void *sound_user;
} VmCore;
bool vm_init(VmCore *, const char *data_dir, uint64_t seed, const JCClock *,
             const JCConfig *, int forced_scene);
bool vm_step(VmCore *);
bool vm_advance(VmCore *, double elapsed, const JCClock *);
void vm_shutdown(VmCore *);
/* Validate captured GPU command stream against the software semantic canvas. */
bool vm_verify_frame(VmCore *, uint8_t *scratch);
#endif
