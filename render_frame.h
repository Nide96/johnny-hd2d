#ifndef JC_RENDER_FRAME_H
#define JC_RENDER_FRAME_H
#include "gfx.h"

enum { JC_ATLAS_W = 2048, JC_ATLAS_H = 4096, JC_DRAW_MAX = 1024 };
typedef struct {
  uint64_t key;
  int x, y, w, h, ax, ay;
  uint8_t flags;
  uint16_t source_sprite;
  char resource[14]; /* owned: a TTM slot may unload before the next render */
  bool walk;
  GfxDomain domain;
  bool transparent;
  bool line;
  int line_x1, line_y1, line_x2, line_y2;
  uint8_t line_color;
  Clip clip;
} JCDraw;
typedef struct {
  uint8_t *atlas;
  Canvas *scratch;
  JCDraw scenery[3];
  size_t scenery_count;
  JCDraw actors[JC_DRAW_MAX];
  size_t actor_count;
  int origin_x, origin_y;
  bool modern_allowed, modern_compatible, walk_preview;
  unsigned scene_id;
  JCDraw draws[JC_DRAW_MAX];
  size_t count;
  int pack_x, pack_y, row_h, height;
  bool overflow;
  uint64_t serial, cut_id;
} JCRenderFrame;

bool jc_render_frame_init(JCRenderFrame *);
void jc_render_frame_free(JCRenderFrame *);
/* Base is an opaque snapshot. Non-sprite operations flatten the prefix of the
 * draw stream; this preserves GDI copy/restore semantics and exact layer order.
 */
void jc_render_frame_base(JCRenderFrame *, const uint8_t *pixels);
void jc_render_frame_command(JCRenderFrame *, const GfxCommand *);
void jc_render_frame_position(const JCRenderFrame *current,
                              const JCRenderFrame *previous, size_t draw,
                              float alpha, bool interpolate, float *x,
                              float *y);
void jc_render_actor_position(const JCRenderFrame *, const JCRenderFrame *,
                              size_t, float, bool, float *, float *);
/* CPU reconstruction of the GPU draw stream for lossless regression checks. */
void jc_render_frame_raster(const JCRenderFrame *, uint8_t *output);
#endif
