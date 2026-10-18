#ifndef JC_SPRITE_MATERIAL_H
#define JC_SPRITE_MATERIAL_H
#include "render_frame.h"
#include "generated/shadow_masks.h"
#include <string.h>
/* Masks describe inspected index-8 components, never all grey pixels. */
static inline const JCShadowMask *jc_sprite_shadow_mask(const JCDraw *d) {
  for (size_t i = 0; i < sizeof jc_shadow_masks / sizeof *jc_shadow_masks;
       ++i) {
    const JCShadowMask *m = &jc_shadow_masks[i];
    if (m->id == d->source_sprite && m->w == d->w && m->h == d->h &&
        !strcmp(m->resource, d->resource))
      return m;
  }
  return NULL;
}
static inline bool jc_sprite_shadow_pixel(const JCShadowMask *m, int x, int y) {
  if (!m)
    return false;
  for (unsigned i = m->start; i < m->start + m->count; ++i) {
    const JCShadowSpan *p = &jc_shadow_spans[i];
    if (p->y > y)
      break;
    if (p->y == y && x >= p->x1 && x < p->x2)
      return true;
  }
  return false;
}
/* Unreviewed grey contact regions keep their original pixels and own their
 * shadow. Do not add a second projected shadow or erase grey props by guess. */
static inline bool jc_sprite_unhandled_shadow(const JCRenderFrame *f,
                                              const JCDraw *d) {
  if (!d->resource[0] || jc_sprite_shadow_mask(d))
    return false;
  unsigned count = 0;
  int left = d->w, right = 0;
  for (int y = d->h * 7 / 10; y < d->h; ++y)
    for (int x = 0; x < d->w; ++x)
      if (f->atlas[(size_t)(d->ay + y) * JC_ATLAS_W + d->ax + x] == 8) {
        ++count;
        if (x < left)
          left = x;
        if (x + 1 > right)
          right = x + 1;
      }
  return count >= 16 && right - left >= 8;
}
typedef struct {
  int left, top, right, bottom;
} JCVisibleBounds;
static inline JCVisibleBounds jc_sprite_body_bounds(const JCRenderFrame *f,
                                                    const JCDraw *d) {
  JCVisibleBounds b = {d->w, d->h, 0, 0};
  const JCShadowMask *m = jc_sprite_shadow_mask(d);
  for (int y = 0; y < d->h; ++y)
    for (int x = 0; x < d->w; ++x) {
      uint8_t v = f->atlas[(size_t)(d->ay + y) * JC_ATLAS_W + d->ax + x];
      if (!v || (v == 8 && jc_sprite_shadow_pixel(m, x, y)))
        continue;
      int xx = (d->flags & FLIP_H) ? d->w - x - 1 : x;
      int yy = (d->flags & FLIP_V) ? d->h - y - 1 : y;
      if (xx < b.left)
        b.left = xx;
      if (yy < b.top)
        b.top = yy;
      if (xx + 1 > b.right)
        b.right = xx + 1;
      if (yy + 1 > b.bottom)
        b.bottom = yy + 1;
    }
  return b;
}
static inline unsigned jc_sprite_strip_shadow(uint8_t *atlas, const JCDraw *d) {
  const JCShadowMask *m = jc_sprite_shadow_mask(d);
  if (!m)
    return 0;
  unsigned count = 0;
  for (unsigned i = m->start; i < m->start + m->count; ++i) {
    const JCShadowSpan *p = &jc_shadow_spans[i];
    for (int x = p->x1; x < p->x2; ++x) {
      uint8_t *v = &atlas[(size_t)(d->ay + p->y) * JC_ATLAS_W + d->ax + x];
      if (*v == 8) {
        *v = 0;
        ++count;
      }
    }
  }
  return count;
}
#endif
