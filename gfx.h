/* Indexed GDI-compatible canvas; adapted from MIT open-scrantic. */
#ifndef JC_GFX_H
#define JC_GFX_H
#include "resource.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum { WIDTH = 640, HEIGHT = 480, FLIP_V = 1, FLIP_H = 2 };
typedef struct {
  int32_t x1, y1, x2, y2;
} Clip;
extern const Clip CLIP_FULL;
/* Trace is optional and non-owning, in composition order after rasterization.
 * result points at the destination canvas; sprite/result are valid only during
 * the callback. Copy image data there if presentation needs it later. */
typedef enum {
  GFX_SPRITE,
  GFX_PIXEL,
  GFX_LINE,
  GFX_RECT,
  GFX_ELLIPSE,
  GFX_COPY,
  GFX_PICTURE,
  GFX_FILL,
  GFX_RESTORE
} GfxOp;
/* Presentation metadata only; never changes the indexed VM raster. */
typedef enum {
  GFX_AMBIENT,
  GFX_ACTOR,
  GFX_RAFT,
  GFX_OCCLUDER,
  GFX_HOLIDAY,
  GFX_WALK,
  GFX_SCENERY
} GfxDomain;
typedef struct {
  GfxOp op;
  GfxDomain domain;
  uint64_t key;
  uint16_t sprite_id;
  const char *resource; /* callback lifetime only; NULL if not identified */
  const uint8_t *result;
  const JCSprite *sprite;
  int32_t x, y, w, h;
  uint8_t flags, fg, bg;
  bool filled;
  Clip clip;
} GfxCommand;
typedef void (*GfxTrace)(void *, const GfxCommand *);
typedef struct {
  uint8_t px[WIDTH * HEIGHT];
  GfxTrace trace;
  void *trace_user;
  uint64_t trace_key;
  GfxDomain trace_domain;
  uint16_t trace_sprite_id;
  const char *trace_resource;
} Canvas;
Clip clip_rect(int32_t x, int32_t y, int32_t w, int32_t h);
void canvas_fill(Canvas *, uint8_t);
void canvas_load_picture(Canvas *, const JCPicture *);
void canvas_copy_rect_from(Canvas *, const Canvas *, int32_t x, int32_t y,
                           int32_t w, int32_t h);
uint8_t *canvas_save_rect(const Canvas *, Clip, size_t *len);
void canvas_restore_rect(Canvas *, Clip, const uint8_t *, size_t len);
void canvas_sprite(Canvas *, const JCSprite *, int32_t x, int32_t y,
                   uint8_t flags, Clip);
void canvas_pixel(Canvas *, int32_t x, int32_t y, uint8_t, Clip);
void canvas_line(Canvas *, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                 uint8_t, Clip);
void canvas_rect(Canvas *, int32_t x, int32_t y, int32_t w, int32_t h,
                 uint8_t fg, uint8_t bg, bool filled, Clip);
void canvas_ellipse(Canvas *, int32_t cx, int32_t cy, int32_t r, uint8_t fg,
                    uint8_t bg, Clip);
#endif
