/* C99 adaptation of open-scrantic gfx.rs (MIT, Glauber Lima).
 * Original GDI semantics, including end-exclusive LineTo and index-0
 * transparency. */
#include "gfx.h"
#include <stdlib.h>
#include <string.h>
const Clip CLIP_FULL = {0, 0, WIDTH - 1, HEIGHT - 1};
static int32_t lo(int32_t a, int32_t b) { return a < b ? a : b; }
static int32_t hi(int32_t a, int32_t b) { return a > b ? a : b; }
static Clip bound(Clip c) {
  c.x1 = hi(c.x1, 0);
  c.y1 = hi(c.y1, 0);
  c.x2 = lo(c.x2, WIDTH - 1);
  c.y2 = lo(c.y2, HEIGHT - 1);
  return c;
}
static bool visible(Clip c) { return c.x1 <= c.x2 && c.y1 <= c.y2; }
static void trace(Canvas *c, GfxCommand cmd) {
  cmd.key = c->trace_key++;
  cmd.domain = c->trace_domain;
  cmd.sprite_id = c->trace_sprite_id;
  cmd.resource = c->trace_resource;
  cmd.result = c->px;
  if (c->trace)
    c->trace(c->trace_user, &cmd);
}
static void put(Canvas *c, int32_t x, int32_t y, uint8_t v, Clip clip) {
  if (x >= clip.x1 && x <= clip.x2 && y >= clip.y1 && y <= clip.y2)
    c->px[y * WIDTH + x] = v;
}
Clip clip_rect(int32_t x, int32_t y, int32_t w, int32_t h) {
  Clip c = {x, y, x + w - 1, y + h - 1};
  return c;
}
static void fill_raw(Canvas *c, uint8_t v) { memset(c->px, v, sizeof c->px); }
static void load_picture_raw(Canvas *c, const JCPicture *p) {
  int y, w = lo(p->w, WIDTH), h = lo(p->h, HEIGHT);
  if (w <= 0 || h <= 0)
    return;
  for (y = 0; y < h; ++y)
    memcpy(c->px + y * WIDTH, p->px + y * p->w, (size_t)w);
}
static void copy_rect_from_raw(Canvas *c, const Canvas *s, int32_t x, int32_t y,
                               int32_t w, int32_t h) {
  int yy;
  Clip r = bound(clip_rect(x, y, w, h));
  if (!visible(r))
    return;
  for (yy = r.y1; yy <= r.y2; ++yy)
    memmove(c->px + yy * WIDTH + r.x1, s->px + yy * WIDTH + r.x1,
            (size_t)(r.x2 - r.x1 + 1));
}
uint8_t *canvas_save_rect(const Canvas *c, Clip r, size_t *len) {
  uint8_t *p;
  int y;
  size_t w, i = 0;
  r = bound(r);
  *len = 0;
  if (!visible(r))
    return NULL;
  w = (size_t)(r.x2 - r.x1 + 1);
  *len = w * (size_t)(r.y2 - r.y1 + 1);
  p = malloc(*len);
  if (!p)
    return NULL;
  for (y = r.y1; y <= r.y2; ++y) {
    memcpy(p + i, c->px + y * WIDTH + r.x1, w);
    i += w;
  }
  *len = i;
  return p;
}
static void restore_rect_raw(Canvas *c, Clip r, const uint8_t *p, size_t len) {
  int y;
  size_t w, i = 0;
  r = bound(r);
  if (!visible(r) || !p)
    return;
  w = (size_t)(r.x2 - r.x1 + 1);
  for (y = r.y1; y <= r.y2 && i + w <= len; ++y) {
    memcpy(c->px + y * WIDTH + r.x1, p + i, w);
    i += w;
  }
}
static void sprite_raw(Canvas *c, const JCSprite *s, int32_t x, int32_t y,
                       uint8_t f, Clip clip) {
  int32_t dx, dy, x0, y0, x1, y1;
  clip = bound(clip);
  x0 = hi(x, clip.x1);
  y0 = hi(y, clip.y1);
  x1 = lo(x + s->w - 1, clip.x2);
  y1 = lo(y + s->h - 1, clip.y2);
  for (dy = y0; dy <= y1; ++dy) {
    int32_t sy = f & FLIP_V ? s->h - 1 - (dy - y) : dy - y;
    for (dx = x0; dx <= x1; ++dx) {
      int32_t sx = f & FLIP_H ? s->w - 1 - (dx - x) : dx - x;
      uint8_t v = s->px[sy * s->w + sx];
      if (v)
        c->px[dy * WIDTH + dx] = v;
    }
  }
}
static void pixel_raw(Canvas *c, int32_t x, int32_t y, uint8_t v, Clip clip) {
  put(c, x, y, v, bound(clip));
}
static void line_raw(Canvas *c, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                     uint8_t v, Clip clip) {
  int32_t dx = abs(x2 - x1), dy = -abs(y2 - y1), sx = (x2 > x1) - (x2 < x1),
          sy = (y2 > y1) - (y2 < y1), err = dx + dy;
  clip = bound(clip);
  while (x1 != x2 || y1 != y2) {
    int32_t e2 = 2 * err;
    put(c, x1, y1, v, clip);
    if (e2 >= dy) {
      err += dy;
      x1 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y1 += sy;
    }
  }
}
static void rect_raw(Canvas *c, int32_t x, int32_t y, int32_t w, int32_t h,
                     uint8_t fg, uint8_t bg, bool filled, Clip clip) {
  int32_t xx, yy;
  clip = bound(clip);
  if (w <= 0 || h <= 0)
    return;
  for (yy = hi(y, clip.y1); yy <= lo(y + h - 1, clip.y2); ++yy)
    for (xx = hi(x, clip.x1); xx <= lo(x + w - 1, clip.x2); ++xx) {
      bool edge = xx == x || yy == y || xx == x + w - 1 || yy == y + h - 1;
      if (edge)
        put(c, xx, yy, fg, clip);
      else if (filled)
        put(c, xx, yy, bg, clip);
    }
}
static bool in_ellipse(int32_t px, int32_t py, int32_t cx, int32_t cy,
                       int32_t r) {
  float x = (float)px + .5f - (float)cx, y = (float)py + .5f - (float)cy;
  return x * x + y * y <= (float)r * (float)r;
}
static void ellipse_raw(Canvas *c, int32_t cx, int32_t cy, int32_t r,
                        uint8_t fg, uint8_t bg, Clip clip) {
  int32_t x, y;
  clip = bound(clip);
  if (r <= 0)
    return;
  for (y = hi(cy - r, clip.y1); y < lo(cy + r, clip.y2 + 1); ++y)
    for (x = hi(cx - r, clip.x1); x < lo(cx + r, clip.x2 + 1); ++x) {
      bool edge;
      if (!in_ellipse(x, y, cx, cy, r))
        continue;
      edge = !in_ellipse(x - 1, y, cx, cy, r) ||
             !in_ellipse(x + 1, y, cx, cy, r) ||
             !in_ellipse(x, y - 1, cx, cy, r) ||
             !in_ellipse(x, y + 1, cx, cy, r);
      put(c, x, y, edge ? fg : bg, clip);
    }
}

void canvas_fill(Canvas *c, uint8_t fg) {
  GfxCommand q = {0};
  q.op = GFX_FILL;
  q.fg = fg;
  q.clip = CLIP_FULL;
  fill_raw(c, fg);
  trace(c, q);
}
void canvas_load_picture(Canvas *c, const JCPicture *p) {
  GfxCommand q = {0};
  q.op = GFX_PICTURE;
  q.w = p->w;
  q.h = p->h;
  q.clip = CLIP_FULL;
  load_picture_raw(c, p);
  trace(c, q);
}
void canvas_copy_rect_from(Canvas *c, const Canvas *s, int32_t x, int32_t y,
                           int32_t w, int32_t h) {
  GfxCommand q = {0};
  q.op = GFX_COPY;
  q.x = x;
  q.y = y;
  q.w = w;
  q.h = h;
  q.clip = clip_rect(x, y, w, h);
  copy_rect_from_raw(c, s, x, y, w, h);
  trace(c, q);
}
void canvas_restore_rect(Canvas *c, Clip r, const uint8_t *p, size_t n) {
  GfxCommand q = {0};
  q.op = GFX_RESTORE;
  q.x = r.x1;
  q.y = r.y1;
  q.w = r.x2 - r.x1 + 1;
  q.h = r.y2 - r.y1 + 1;
  q.clip = r;
  restore_rect_raw(c, r, p, n);
  trace(c, q);
}
void canvas_sprite(Canvas *c, const JCSprite *s, int32_t x, int32_t y,
                   uint8_t f, Clip clip) {
  GfxCommand q = {0};
  q.op = GFX_SPRITE;
  q.sprite = s;
  q.x = x;
  q.y = y;
  q.w = s->w;
  q.h = s->h;
  q.flags = f;
  q.clip = clip;
  sprite_raw(c, s, x, y, f, clip);
  trace(c, q);
}
void canvas_pixel(Canvas *c, int32_t x, int32_t y, uint8_t v, Clip clip) {
  GfxCommand q = {0};
  q.op = GFX_PIXEL;
  q.x = x;
  q.y = y;
  q.w = q.h = 1;
  q.fg = v;
  q.clip = clip;
  pixel_raw(c, x, y, v, clip);
  trace(c, q);
}
void canvas_line(Canvas *c, int32_t x, int32_t y, int32_t x2, int32_t y2,
                 uint8_t v, Clip clip) {
  GfxCommand q = {0};
  q.op = GFX_LINE;
  q.x = x;
  q.y = y;
  q.w = x2;
  q.h = y2;
  q.fg = v;
  q.clip = clip;
  line_raw(c, x, y, x2, y2, v, clip);
  trace(c, q);
}
void canvas_rect(Canvas *c, int32_t x, int32_t y, int32_t w, int32_t h,
                 uint8_t fg, uint8_t bg, bool filled, Clip clip) {
  GfxCommand q = {0};
  q.op = GFX_RECT;
  q.x = x;
  q.y = y;
  q.w = w;
  q.h = h;
  q.fg = fg;
  q.bg = bg;
  q.filled = filled;
  q.clip = clip;
  rect_raw(c, x, y, w, h, fg, bg, filled, clip);
  trace(c, q);
}
void canvas_ellipse(Canvas *c, int32_t x, int32_t y, int32_t r, uint8_t fg,
                    uint8_t bg, Clip clip) {
  GfxCommand q = {0};
  q.op = GFX_ELLIPSE;
  q.x = x;
  q.y = y;
  q.w = r;
  q.fg = fg;
  q.bg = bg;
  q.clip = clip;
  ellipse_raw(c, x, y, r, fg, bg, clip);
  trace(c, q);
}
