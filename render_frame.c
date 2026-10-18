#include "render_frame.h"
#include "generated/walk_anchors.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool jc_render_frame_init(JCRenderFrame *f) {
  memset(f, 0, sizeof(*f));
  f->atlas = calloc((size_t)JC_ATLAS_W, JC_ATLAS_H);
  f->scratch = calloc(1, sizeof(*f->scratch));
  if (!f->atlas || !f->scratch) {
    jc_render_frame_free(f);
    return false;
  }
  return true;
}
void jc_render_frame_free(JCRenderFrame *f) {
  free(f->atlas);
  free(f->scratch);
  memset(f, 0, sizeof(*f));
}
static void flatten(JCRenderFrame *f, const uint8_t *pixels) {
  f->count = 1;
  f->draws[0] =
      (JCDraw){.w = WIDTH, .h = HEIGHT, .clip = {0, 0, WIDTH - 1, HEIGHT - 1}};
  for (int y = 0; y < HEIGHT; ++y)
    memcpy(f->atlas + (size_t)y * JC_ATLAS_W, pixels + (size_t)y * WIDTH,
           WIDTH);
}
void jc_render_frame_base(JCRenderFrame *f, const uint8_t *pixels) {
  f->count = 1;
  f->actor_count = 0;
  f->scenery_count = 0;
  f->modern_allowed = false;
  f->modern_compatible = true;
  f->pack_x = 0;
  f->pack_y = HEIGHT + 1;
  f->row_h = 0;
  f->height = HEIGHT;
  f->overflow = false;
  f->draws[0] =
      (JCDraw){.w = WIDTH, .h = HEIGHT, .clip = {0, 0, WIDTH - 1, HEIGHT - 1}};
  for (int y = 0; y < HEIGHT; ++y)
    memcpy(f->atlas + (size_t)y * JC_ATLAS_W, pixels + (size_t)y * WIDTH,
           WIDTH);
}
static void capture_primitive(JCRenderFrame *, const GfxCommand *);
void jc_render_frame_command(JCRenderFrame *f, const GfxCommand *c) {
  if (c->op != GFX_SPRITE) {
    if (c->result)
      flatten(f, c->result);
    if (c->domain == GFX_ACTOR)
      capture_primitive(f, c);
    return;
  }
  if (f->overflow)
    return;
  const JCSprite *s = c->sprite;
  if (!s || s->w <= 0 || s->h <= 0)
    return;
  if (f->pack_x + s->w + 1 > JC_ATLAS_W) {
    f->pack_x = 0;
    f->pack_y += f->row_h + 1;
    f->row_h = 0;
  }
  if (s->w > JC_ATLAS_W || f->pack_y + s->h > JC_ATLAS_H ||
      f->count == JC_DRAW_MAX) {
    f->overflow = true;
    return;
  }
  JCDraw value = {.key = c->key,
                  .x = c->x,
                  .y = c->y,
                  .w = s->w,
                  .h = s->h,
                  .ax = f->pack_x,
                  .ay = f->pack_y,
                  .flags = c->flags,
                  .source_sprite = c->sprite_id,
                  .walk = c->domain == GFX_WALK,
                  .domain = c->domain,
                  .transparent = true,
                  .clip = c->clip};
  JCDraw *d = &value;
  if (c->resource) {
    size_t n = 0;
    while (n + 1 < sizeof value.resource && c->resource[n]) {
      value.resource[n] = c->resource[n];
      ++n;
    }
    value.resource[n] = '\0';
  }
  if (c->domain == GFX_SCENERY) {
    if (f->scenery_count == 3) {
      f->overflow = true;
      return;
    }
    f->scenery[f->scenery_count++] = value;
  } else {
    f->draws[f->count++] = value;
    if (c->domain == GFX_ACTOR || c->domain == GFX_WALK ||
        c->domain == GFX_OCCLUDER || c->domain == GFX_HOLIDAY ||
        c->domain == GFX_RAFT) {
      if (f->actor_count == JC_DRAW_MAX) {
        f->overflow = true;
        return;
      }
      f->actors[f->actor_count++] = value;
    }
  }
  for (int y = 0; y < s->h; ++y)
    memcpy(f->atlas + (size_t)(d->ay + y) * JC_ATLAS_W + d->ax,
           s->px + (size_t)y * s->w, (size_t)s->w);
  f->pack_x += s->w + 1;
  if (s->h > f->row_h)
    f->row_h = s->h;
  if (f->pack_y + s->h > f->height)
    f->height = f->pack_y + s->h;
}
void jc_render_frame_position(const JCRenderFrame *cur,
                              const JCRenderFrame *prev, size_t index,
                              float alpha, bool interpolate, float *x,
                              float *y) {
  const JCDraw *d = &cur->draws[index];
  *x = (float)d->x;
  *y = (float)d->y;
  if (!interpolate || !d->transparent || !d->key || alpha >= 1.0f)
    return;
  const JCDraw *old = NULL;
  for (size_t i = 1; i < prev->count; ++i)
    if (prev->draws[i].key == d->key) {
      if (old)
        return;
      old = &prev->draws[i];
    }
  if (!old || strcmp(d->resource, old->resource) || abs(d->x - old->x) > 48 ||
      abs(d->y - old->y) > 48 || memcmp(&d->clip, &old->clip, sizeof(Clip)))
    return;
  *x = (float)old->x + ((float)d->x - (float)old->x) * alpha;
  *y = (float)old->y + ((float)d->y - (float)old->y) * alpha;
}
void jc_render_frame_raster(const JCRenderFrame *f, uint8_t *out) {
  memset(out, 0, WIDTH * HEIGHT);
  for (size_t i = 0; i < f->count; ++i) {
    const JCDraw *d = &f->draws[i];
    for (int yy = 0; yy < d->h; ++yy) {
      int y = d->y + yy;
      if (y < 0 || y >= HEIGHT || y < d->clip.y1 || y > d->clip.y2)
        continue;
      int sy = (d->flags & FLIP_V) ? d->h - 1 - yy : yy;
      for (int xx = 0; xx < d->w; ++xx) {
        int x = d->x + xx;
        if (x < 0 || x >= WIDTH || x < d->clip.x1 || x > d->clip.x2)
          continue;
        int sx = (d->flags & FLIP_H) ? d->w - 1 - xx : xx;
        uint8_t v = f->atlas[(size_t)(d->ay + sy) * JC_ATLAS_W + d->ax + sx];
        if (v || !d->transparent)
          out[(size_t)y * WIDTH + x] = v;
      }
    }
  }
}

/* Keep explicit drawing (e.g. a fishing line), excluding VM buffer copies.
 * Rasterize with the same GDI semantics before retaining a compact patch. */
static void capture_primitive(JCRenderFrame *f, const GfxCommand *c) {
  Canvas *s = f->scratch;
  if (c->op == GFX_COPY || c->op == GFX_RESTORE)
    return;
  if (c->op == GFX_PICTURE || c->op == GFX_FILL) {
    f->modern_compatible = false;
    return;
  }
  /* Index zero is transparent in the sprite pipeline. Opaque black primitives
   * need a coverage texture; use the exact classic path until that is added. */
  if (c->fg == 0 ||
      ((c->op == GFX_ELLIPSE || (c->op == GFX_RECT && c->filled)) &&
       c->bg == 0)) {
    f->modern_compatible = false;
    return;
  }
  memset(s->px, 0, sizeof s->px);
  switch (c->op) {
  case GFX_PIXEL:
    canvas_pixel(s, c->x, c->y, c->fg, c->clip);
    break;
  case GFX_LINE:
    canvas_line(s, c->x, c->y, c->w, c->h, c->fg, c->clip);
    break;
  case GFX_RECT:
    canvas_rect(s, c->x, c->y, c->w, c->h, c->fg, c->bg, c->filled, c->clip);
    break;
  case GFX_ELLIPSE:
    canvas_ellipse(s, c->x, c->y, c->w, c->fg, c->bg, c->clip);
    break;
  default:
    return;
  }
  int x1 = WIDTH, y1 = HEIGHT, x2 = -1, y2 = -1;
  for (int y = 0; y < HEIGHT; ++y)
    for (int x = 0; x < WIDTH; ++x)
      if (s->px[y * WIDTH + x]) {
        if (x < x1)
          x1 = x;
        if (x > x2)
          x2 = x;
        if (y < y1)
          y1 = y;
        if (y > y2)
          y2 = y;
      }
  if (x2 < x1)
    return;
  int w = x2 - x1 + 1, h = y2 - y1 + 1;
  if (w * h > WIDTH * HEIGHT / 4) {
    f->modern_compatible = false;
    return;
  }
  uint8_t *px = malloc((size_t)w * h);
  if (!px) {
    f->overflow = true;
    return;
  }
  for (int y = 0; y < h; ++y)
    memcpy(px + (size_t)y * w, s->px + (y1 + y) * WIDTH + x1, (size_t)w);
  JCSprite patch = {w, h, px};
  GfxCommand q = {.op = GFX_SPRITE,
                  .domain = GFX_ACTOR,
                  .key = c->key,
                  .sprite = &patch,
                  .x = x1,
                  .y = y1,
                  .clip = c->clip};
  size_t old_count = f->count;
  size_t old_actors = f->actor_count;
  jc_render_frame_command(f, &q);
  f->count = old_count; /* Patch belongs only to the modern retained list. */
  if (c->op == GFX_LINE && f->actor_count == old_actors + 1) {
    JCDraw *d = &f->actors[old_actors];
    d->line = true;
    d->line_x1 = c->x;
    d->line_y1 = c->y;
    d->line_x2 = c->w;
    d->line_y2 = c->h;
    d->line_color = c->fg;
  }
  free(px);
}
void jc_render_actor_position(const JCRenderFrame *cur,
                              const JCRenderFrame *prev, size_t i, float a,
                              bool interpolate, float *x, float *y) {
  const JCDraw *d = &cur->actors[i], *old = NULL;
  *x = (float)d->x;
  *y = (float)d->y;
  if (!interpolate || !d->key || a >= 1)
    return;
  for (size_t n = 0; n < prev->actor_count; ++n)
    if (prev->actors[n].key == d->key) {
      if (old)
        return;
      old = &prev->actors[n];
    }
  if (!old || strcmp(d->resource, old->resource) || old->flags != d->flags ||
      abs(d->x - old->x) > 48 || abs(d->y - old->y) > 48 ||
      memcmp(&d->clip, &old->clip, sizeof(Clip)))
    return;
  if (!strcmp(d->resource, "JOHNWALK.BMP") && d->source_sprite < 36 &&
      old->source_sprite < 36 && !(d->flags & FLIP_V)) {
    float now_pivot = walk_anchors[d->source_sprite].pivot;
    float old_pivot = walk_anchors[old->source_sprite].pivot;
    if (d->flags & FLIP_H) {
      now_pivot = d->w - now_pivot;
      old_pivot = old->w - old_pivot;
    }
    /* Interpolate the same anatomical anchor, not rectangles whose padding,
     * width and height change with the pose. Apply the current pose afterwards.
     */
    *x = old->x + old_pivot + (d->x + now_pivot - old->x - old_pivot) * a -
         now_pivot;
    *y = old->y + old->h + (d->y + d->h - old->y - old->h) * a - d->h;
    return;
  }
  *x = old->x + (d->x - old->x) * a;
  *y = old->y + (d->y - old->y) * a;
}
