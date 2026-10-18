#ifndef JC_SCENE_H
#define JC_SCENE_H
#include "render_frame.h"

/* Recovered two-clock model: 55 ms composition, 16 ms script timer. */
#define VM_STEP_SECONDS (55.0 / 1000.0)
#define MAX_FRAME_SECONDS 0.25
typedef struct {
  JCRenderFrame buffers[2];
  JCRenderFrame *previous, *current;
  double accumulator, visual_seconds, dropped_seconds;
  bool interpolate;
} SceneState;

static inline bool scene_init(SceneState *s) {
  *s = (SceneState){0};
  s->previous = &s->buffers[0];
  s->current = &s->buffers[1];
  if (!jc_render_frame_init(s->previous))
    return false;
  if (!jc_render_frame_init(s->current)) {
    jc_render_frame_free(s->previous);
    return false;
  }
  return true;
}
static inline void scene_free(SceneState *s) {
  jc_render_frame_free(&s->buffers[0]);
  jc_render_frame_free(&s->buffers[1]);
  *s = (SceneState){0};
}
static inline void scene_swap(SceneState *s) {
  JCRenderFrame *old = s->previous;
  s->previous = s->current;
  s->current = old;
}
static inline float scene_alpha(const SceneState *s) {
  return (float)(s->accumulator / VM_STEP_SECONDS);
}
#endif
