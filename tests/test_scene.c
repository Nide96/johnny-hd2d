#include "scene.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void walk_anchor_interpolation(void) {
  JCRenderFrame now = {0}, before = {0};
  now.actor_count = before.actor_count = 1;
  JCDraw *a = &before.actors[0], *b = &now.actors[0];
  *a = (JCDraw){.key = 7,
                .x = 390,
                .y = 216,
                .w = 40,
                .h = 73,
                .source_sprite = 26,
                .clip = CLIP_FULL};
  *b = (JCDraw){.key = 7,
                .x = 391,
                .y = 217,
                .w = 32,
                .h = 72,
                .source_sprite = 27,
                .clip = CLIP_FULL};
  strcpy(a->resource, "JOHNWALK.BMP");
  strcpy(b->resource, "JOHNWALK.BMP");
  float x, y;
  /* Actual post-reading transition: the old head is at 404.5, current at
   * 401.5, both baselines at 289, despite a storage box moving right. */
  for (unsigned i = 0; i <= 4; i++) {
    float alpha = i * .25f;
    jc_render_actor_position(&now, &before, 0, alpha, true, &x, &y);
    assert(fabsf(x + 10.5f - (404.5f - 3.f * alpha)) < .0001f);
    assert(fabsf(y + 72.f - 289.f) < .0001f);
  }
  a->flags = b->flags = FLIP_H;
  jc_render_actor_position(&now, &before, 0, .25f, true, &x, &y);
  assert(fabsf(x + 21.5f - 414.75f) < .0001f && y == 217.f);
  a->flags = 0;
  jc_render_actor_position(&now, &before, 0, .25f, true, &x, &y);
  assert(x == 391.f && y == 217.f); /* direction change snaps */
  b->flags = 0;
  jc_render_actor_position(&now, &before, 0, .25f, false, &x, &y);
  assert(x == 391.f && y == 217.f); /* disabled interpolation remains exact */
}
int main(void) {
  walk_anchor_interpolation();
  SceneState s;
  assert(scene_init(&s));
  uint8_t pixels[WIDTH * HEIGHT] = {0}, sprite_px[4] = {1, 0, 2, 3},
                         out[WIDTH * HEIGHT];
  JCSprite sprite = {2, 2, sprite_px};
  jc_render_frame_base(s.current, pixels);
  jc_render_frame_base(s.previous, pixels);
  GfxCommand cmd = {.op = GFX_SPRITE,
                    .domain = GFX_ACTOR,
                    .key = 42,
                    .sprite = &sprite,
                    .x = 10,
                    .y = 10,
                    .clip = {0, 0, 639, 479}};
  jc_render_frame_command(s.previous, &cmd);
  cmd.x = 20;
  jc_render_frame_command(s.current, &cmd);
  float x, y;
  jc_render_frame_position(s.current, s.previous, 1, .5f, true, &x, &y);
  assert(fabsf(x - 15.f) < .0001f && y == 10);
  jc_render_frame_raster(s.current, out);
  assert(out[10 * WIDTH + 20] == 1 && out[10 * WIDTH + 21] == 0 &&
         out[11 * WIDTH + 20] == 2);
  s.current->draws[1].key = 43;
  jc_render_frame_position(s.current, s.previous, 1, .5f, true, &x, &y);
  assert(x == 20);
  s.current->draws[1].key = 42;
  s.current->draws[1].x = 100;
  jc_render_frame_position(s.current, s.previous, 1, .5f, true, &x, &y);
  assert(x == 100);
  memset(pixels, 7, sizeof pixels);
  GfxCommand flatten = {.op = GFX_COPY, .result = pixels};
  jc_render_frame_command(s.current, &flatten);
  assert(s.current->count == 1);
  jc_render_frame_raster(s.current, out);
  assert(!memcmp(out, pixels, sizeof pixels));
  assert(s.current->actor_count ==
         1); /* A prefix flatten must not erase Johnny. */
  jc_render_actor_position(s.current, s.previous, 0, .5f, true, &x, &y);
  assert(x == 15.f && y == 10.f);
  GfxCommand occluder = cmd;
  occluder.domain = GFX_OCCLUDER;
  jc_render_frame_command(s.current, &occluder);
  assert(s.current->actor_count == 2);
  assert(s.current->actors[1].domain == GFX_OCCLUDER);
  jc_render_frame_command(s.current, &flatten);
  GfxCommand line = {.op = GFX_LINE,
                     .domain = GFX_ACTOR,
                     .key = 99,
                     .x = 20,
                     .y = 30,
                     .w = 25,
                     .h = 30,
                     .fg = 2,
                     .clip = CLIP_FULL};
  jc_render_frame_command(s.current, &line);
  assert(s.current->actor_count == 3);
  assert(s.current->actors[2].x == 20 && s.current->actors[2].w == 5);
  const JCDraw *stroke = &s.current->actors[2];
  assert(stroke->line && stroke->line_x1 == 20 && stroke->line_y1 == 30 &&
         stroke->line_x2 == 25 && stroke->line_y2 == 30 &&
         stroke->line_color == 2);
  jc_render_frame_raster(s.current, out);
  assert(!memcmp(out, pixels,
                 sizeof pixels)); /* Explicit line patch is modern-only. */
  jc_render_frame_base(s.current, pixels);
  assert(s.current->actor_count == 0);
  line.fg = 0;
  jc_render_frame_command(s.current, &line);
  assert(!s.current->modern_compatible);
  GfxCommand scenery = cmd;
  scenery.domain = GFX_SCENERY;
  scenery.sprite_id = 13;
  size_t classic_count = s.current->count;
  jc_render_frame_command(s.current, &scenery);
  assert(s.current->scenery_count == 1 && s.current->count == classic_count);
  jc_render_frame_command(s.current, &flatten);
  assert(s.current->scenery_count == 1);
  scene_free(&s);
  puts("capture: keyed interpolation, discontinuity, transparency, prefix "
       "copies OK");
  return 0;
}
