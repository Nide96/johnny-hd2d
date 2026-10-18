#ifndef JC_LAYOUT_H
#define JC_LAYOUT_H
/* One uniform, non-mirrored camera for original island-local coordinates.
 * No per-character or demo translation: interaction points stay attached. */
enum { JC_VIEW_W = 1280, JC_VIEW_H = 720 };
#define JC_WORLD_SCALE 1.8f
static inline float jc_layout_x(float x) {
  return 640.f + (x - 440.f) * JC_WORLD_SCALE;
}
static inline float jc_layout_y(float y) {
  return 450.f + (y - 293.f) * JC_WORLD_SCALE;
}
static inline float jc_layout_width(float w) { return w * JC_WORLD_SCALE; }
#endif
