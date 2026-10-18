#ifndef JC_PALM_LAYOUT_H
#define JC_PALM_LAYOUT_H
#include "render_frame.h"
#include <string.h>
/* Presentation only: TTM uses an alternate silhouette and a taller crown.
 * Reuse the backdrop geometry/atlas mask, preserving script displacement,
 * flags, clipping and ordering. The VM canvas remains untouched. */
static inline JCDraw jc_palm_draw(const JCRenderFrame *f, const JCDraw *d) {
  JCDraw p = *d;
  bool script = !strcmp(d->resource, "TRUNK.BMP");
  unsigned sprite = script ? (d->source_sprite == 0 ? 13 : 12)
                           : d->source_sprite;
  for (size_t i = 0; i < f->scenery_count; ++i) {
    const JCDraw *base = &f->scenery[i];
    if (base->source_sprite != sprite) continue;
    p.ax = base->ax; p.ay = base->ay;
    p.w = base->w; p.h = base->h;
    if (script) {
      p.x = base->x + d->x - (sprite == 13 ? 443 : 365) - f->origin_x;
      p.y = base->y + d->y - (sprite == 13 ? 148 : 122) - f->origin_y;
    }
    break;
  }
  p.source_sprite = (uint16_t)sprite;
  return p;
}
#endif
