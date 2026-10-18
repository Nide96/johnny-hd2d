#include "character_assets.h"
#include "vm_core.h"
#include "ttm.h"
#include "generated/johnny_walk.h"
#include "generated/johnny_actions.h"
#include "generated/johnny_reading.h"
#include "generated/johnny_fishing.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void capture(void *f, const GfxCommand *c) {
  jc_render_frame_command(f, c);
}
static void slot_lifetime(const JCAssets *assets) {
  JCBmp walk = {0};
  JCError error = {{0}};
  assert(jc_archive_bmp(&assets->archive, "JOHNWALK.BMP", &walk, &error));
  Canvas *canvas = calloc(1, sizeof *canvas);
  uint8_t *pixels = malloc(WIDTH * HEIGHT);
  JCRenderFrame f;
  assert(canvas && pixels && jc_render_frame_init(&f));
  jc_render_frame_base(&f, canvas->px);
  canvas->trace = capture;
  canvas->trace_user = &f;
  canvas->trace_domain = GFX_ACTOR;
  canvas->trace_resource = "CALLER.BMP";
  canvas->trace_sprite_id = 99;
  JCOp ops[] = {{0xF02F, {0}, "johnwalk.bmp"},
                {0xA504, {10, 10, 7, 0, 0, 0}, NULL},
                {0x0080, {0}, NULL},
                {0xF02F, {0}, "STNDLAY.BMP"},
                {0xA504, {100, 100, 2, 0, 0, 0}, NULL},
                {0x0080, {0}, NULL}};
  JCFrame frame = {ops, 6};
  TtmCtx ctx = {0};
  TtmScene scene;
  TtmFrameResult result;
  ctx.ttm.frames = &frame;
  ctx.ttm.frame_count = 1;
  VmEnv env = {.work = canvas,
               .bg = canvas,
               .front = canvas,
               .archive = &assets->archive,
               .johnwalk = &walk};
  ttm_scene_init(&scene, 0, 1, 1, 0);
  assert(ttm_run_frame(&ctx, &scene, &env, &result) && !env.failed);
  assert(!ctx.bmps[0] && !ctx.bmp_names[0][0]);
  assert(!strcmp(canvas->trace_resource, "CALLER.BMP") &&
         canvas->trace_sprite_id == 99);
  assert(f.actor_count == 2);
  assert(!strcmp(f.actors[0].resource, "JOHNWALK.BMP") &&
         f.actors[0].source_sprite == 7);
  assert(!strcmp(f.actors[1].resource, "STNDLAY.BMP") &&
         f.actors[1].source_sprite == 2);
  assert(jc_character_pose(&f.actors[0]).sheet == JC_HD_WALK);
  assert(jc_character_pose(&f.actors[1]).sheet == JC_HD_REST);
  jc_render_frame_raster(&f, pixels);
  assert(!memcmp(pixels, canvas->px, WIDTH * HEIGHT));
  ctx.ttm.frames = NULL;
  ctx.ttm.frame_count = 0;
  ttm_ctx_free(&ctx);
  jc_bmp_free(&walk);
  /* Captured images and names survive resource/context destruction. */
  assert(!strcmp(f.actors[1].resource, "STNDLAY.BMP"));
  jc_render_frame_raster(&f, pixels);
  assert(!memcmp(pixels, canvas->px, WIDTH * HEIGHT));
  jc_render_frame_free(&f);
  free(canvas);
  free(pixels);
}
static void lookup(void) {
  JCDraw d = {0};
  strcpy(d.resource, "JOHNWALK.BMP");
  unsigned count = 0;
  for (unsigned i = 0; i < 36; i++) {
    d.source_sprite = (uint16_t)i;
    count += jc_character_pose(&d).sheet != JC_HD_NONE;
  }
  assert(count == 24);
  d.source_sprite = 13;
  assert(jc_character_pose(&d).sheet == JC_HD_NONE);
  d.source_sprite = 11;
  assert(jc_character_pose(&d).mirror);
  strcpy(d.resource, "STNDLAY.BMP");
  d.source_sprite = 8;
  assert(jc_character_pose(&d).sheet == JC_HD_NONE); /* isolated hand */
  strcpy(d.resource, "SLEEP.BMP");
  d.source_sprite = 12;
  assert(jc_character_pose(&d).sheet == JC_HD_NONE); /* isolated head */
  strcpy(d.resource, "MJFISH1.BMP");
  d.source_sprite = 1;
  assert(jc_character_pose(&d).sheet == JC_HD_FISH &&
         !jc_character_pose(&d).shadow); /* upper body only */
  strcpy(d.resource, "LITEBULB.BMP");
  for (unsigned id = 0; id < 5; id++) {
    d.source_sprite = (uint16_t)id;
    assert(jc_symbol_kind(&d) == (id == 1   ? JC_SYMBOL_EXCLAMATION
                                  : id == 4 ? JC_SYMBOL_QUESTION
                                            : JC_SYMBOL_NONE));
    assert(jc_character_pose(&d).sheet == JC_HD_NONE);
  }
  strcpy(d.resource, "JOHNWALK.BMP");
  d.source_sprite = 24;
  assert(jc_character_pose(&d).sheet == JC_HD_READ_WALK &&
         !jc_character_pose(&d).mirror);
  assert(jc_symbol_kind(&d) == JC_SYMBOL_NONE);
  d.resource[0] = 0;
  d.walk = true;
  assert(jc_character_pose(&d).sheet == JC_HD_NONE); /* no shape guessing */
  const JCHDCell *tables[] = {
      hd_cells,           hd_pose_cells, hd_rest_cells,      hd_sleep_cells,
      hd_read_walk_cells, hd_read_cells, hd_read_props_cells,
      hd_fishing_cells, hd_fishing_parts_cells, hd_fishing_reactions_cells, hd_fishing_reaction_parts_cells};
  const unsigned sizes[] = {8, 10, 8, 16, 6, 24, 12, 16, 14, 12, 15};
  for (unsigned t = 0; t < sizeof sizes / sizeof *sizes; t++)
    for (unsigned n = 0; n < sizes[t]; n++) {
      const JCHDCell *c = &tables[t][n];
      assert(c->x >= 0 && c->y >= 0 && c->w > 0 && c->h > 0);
      assert(c->x + c->w <= 1536 && c->y + c->h <= 1024);
      assert(c->pivot >= 0 && c->pivot <= c->w);
    }
}
int main(int argc, char **argv) {
  VmCore *v = calloc(1, sizeof *v);
  uint8_t *scratch = malloc(WIDTH * HEIGHT);
  JCClock clock = {2026, 10, 18, 12, 0};
  JCConfig cfg = jc_config_default();
  cfg.num_days = cfg.intro = 1;
  lookup();
  assert(v && scratch &&
         vm_init(v, argc > 1 ? argv[1] : "assets/original", 4242, &clock, &cfg,
                 51));
  slot_lifetime(&v->assets);
  unsigned seen = 0, steps = 0;
  do {
    assert(++steps < 6000 && vm_step(v) && vm_verify_frame(v, scratch));
    for (size_t i = 0; i < v->scene.current->actor_count; i++) {
      JCHDPose p = jc_character_pose(&v->scene.current->actors[i]);
      if (p.sheet >= 0)
        seen |= 1u << p.sheet;
    }
  } while (!jc_engine_scenes_done(v->engine));
  assert(seen == 15);
  vm_shutdown(v);
  free(v);
  free(scratch);
  puts(
      "HD characters: 48 base poses, partial-body exclusions, slot "
      "reuse/unload "
      "ownership, real scene 51 with all 4 sheets and exact classic pixels OK");
}
