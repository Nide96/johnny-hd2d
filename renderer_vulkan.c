#include "renderer_vulkan.h"
#include "generated/palette.glsl.h"
#include "generated/modern.glsl.h"
#include "generated/foliage.glsl.h"
#include "layout.h"
#include "palm_layout.h"
#include "character_assets.h"
#include "hybrid.h"
#include "environment.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct Renderer {
  sg_image atlas, palette, lut;
  sg_buffer staging;
  sg_view lut_view;
  sg_pipeline hybrid_pipeline;
  JCHybridLight light;
  double hour, environment_seconds;
  bool environment_enabled;
  float breeze[4], fire[4];
  uint64_t environment_frames, wind_frames, fire_frames;
  sg_image environment_mask;
  sg_view environment_view;
  bool atlas_immutable;
  JCVisibleBounds body[JC_DRAW_MAX];
  bool casts_shadow[JC_DRAW_MAX];
  uint64_t stripped_pixels, line_draws, johnny_draws;
  uint64_t original_palm_draws;
  bool johnny_material[JC_DRAW_MAX];
  size_t max_upload;
  uint64_t fallback_draws, shadow_draws, composite_frames;
  sg_view atlas_view, palette_view;
  sg_sampler nearest;
  sg_shader shader;
  sg_pipeline pipeline;
  uint64_t serial;
  bool uploaded;
  sg_image plate;
  sg_view plate_view;
  sg_sampler linear;
  sg_shader modern_shader;
  sg_pipeline modern_pipeline;
  bool modern_loaded, mode_known, was_modern, blocked;
  uint64_t blocked_cut;
  sg_image palm;
  sg_view palm_view;
  sg_shader foliage_shader;
  sg_pipeline foliage_pipeline;
};
static uint8_t *read_rgba(const char *path, int *width, int *height,
                          size_t *bytes) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    fprintf(stderr, "RGBA texture unavailable: %s\n", path);
    return NULL;
  }
  uint8_t header[12];
  bool ok = fread(header, 1, 12, file) == 12 && !memcmp(header, "JCR1", 4);
  uint32_t w = 0, h = 0;
  if (ok)
    for (unsigned i = 0; i < 4; ++i) {
      w |= (uint32_t)header[4 + i] << (i * 8);
      h |= (uint32_t)header[8 + i] << (i * 8);
    }
  ok = ok && w > 0 && h > 0 && w <= 4096 && h <= 4096;
  size_t size = (size_t)w * h * 4;
  uint8_t *pixels = ok ? malloc(size) : NULL;
  ok = ok && pixels && fread(pixels, 1, size, file) == size &&
       fgetc(file) == EOF;
  fclose(file);
  if (!ok) {
    free(pixels);
    fprintf(stderr, "Invalid RGBA texture: %s\n", path);
    return NULL;
  }
  *width = (int)w;
  *height = (int)h;
  *bytes = size;
  return pixels;
}
static bool load_plate(Renderer *r, const char *path) {
  int w = 0, h = 0;
  size_t size = 0;
  uint8_t *pixels = read_rgba(path, &w, &h, &size);
  bool ok = pixels != NULL;
  if (ok) {
    r->plate =
        sg_make_image(&(sg_image_desc){.width = (int)w,
                                       .height = (int)h,
                                       .pixel_format = SG_PIXELFORMAT_RGBA8,
                                       .data.mip_levels[0] = {pixels, size},
                                       .label = "modern sunset plate"});
    r->plate_view = sg_make_view(&(sg_view_desc){.texture.image = r->plate});
    r->linear =
        sg_make_sampler(&(sg_sampler_desc){.min_filter = SG_FILTER_LINEAR,
                                           .mag_filter = SG_FILTER_LINEAR,
                                           .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                                           .wrap_v = SG_WRAP_CLAMP_TO_EDGE});
    r->modern_shader = sg_make_shader(modern_shader_desc(sg_query_backend()));
    r->modern_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = r->modern_shader, .label = "water and local fire glow"});
    uint8_t *mask = jc_environment_mask(pixels, w, h);
    if (!mask) {
      free(pixels);
      return false;
    }
    r->environment_mask = sg_make_image(&(sg_image_desc){
        .width = JC_ENV_W, .height = JC_ENV_H,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0] = {mask, JC_ENV_W * JC_ENV_H * 4},
        .label = "runtime shore distance / land / rock"});
    r->environment_view = sg_make_view(&(sg_view_desc){
        .texture.image = r->environment_mask});
    free(mask);
    ok = sg_query_image_state(r->plate) == SG_RESOURCESTATE_VALID &&
         sg_query_view_state(r->plate_view) == SG_RESOURCESTATE_VALID &&
         sg_query_sampler_state(r->linear) == SG_RESOURCESTATE_VALID &&
         sg_query_pipeline_state(r->modern_pipeline) == SG_RESOURCESTATE_VALID &&
         sg_query_image_state(r->environment_mask) == SG_RESOURCESTATE_VALID &&
         sg_query_view_state(r->environment_view) == SG_RESOURCESTATE_VALID;
  }
  free(pixels);
  if (!ok)
    fprintf(stderr, "Invalid modern background or GPU resource: %s\n", path);
  r->modern_loaded = ok;
  return ok;
}
static bool load_palm(Renderer *r, const char *path) {
  int w = 0, h = 0;
  size_t size = 0;
  uint8_t *pixels = read_rgba(path, &w, &h, &size);
  if (!pixels)
    return false;
  if (w != 1024 || h != 1536) {
    free(pixels);
    fprintf(stderr, "Palm sheet must be 1024x1536\n");
    return false;
  }
  r->palm =
      sg_make_image(&(sg_image_desc){.width = w,
                                     .height = h,
                                     .pixel_format = SG_PIXELFORMAT_RGBA8,
                                     .data.mip_levels[0] = {pixels, size},
                                     .label = "registered palm material"});
  free(pixels);
  r->palm_view = sg_make_view(&(sg_view_desc){.texture.image = r->palm});
  r->foliage_shader = sg_make_shader(foliage_shader_desc(sg_query_backend()));
  r->foliage_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = r->foliage_shader,
      .colors[0].blend = {.enabled = true,
                          .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                          .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                          .src_factor_alpha = SG_BLENDFACTOR_ONE,
                          .dst_factor_alpha =
                              SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA},
      .label = "palm: original trunk silhouette and depth order"});
  return sg_query_image_state(r->palm) == SG_RESOURCESTATE_VALID &&
         sg_query_view_state(r->palm_view) == SG_RESOURCESTATE_VALID &&
         sg_query_pipeline_state(r->foliage_pipeline) == SG_RESOURCESTATE_VALID;
}
void renderer_set_hour(Renderer *r, double hour) {
  if (r) {
    r->light = jc_hybrid_light(hour);
    r->hour = hour;
  }
}
void renderer_set_environment(Renderer *r, double seconds, bool enabled) {
  if (r) {
    r->environment_seconds = seconds >= 0 ? seconds : 0;
    r->environment_enabled = enabled;
  }
}
Renderer *renderer_create(const uint8_t palette[256][3],
                          const char *modern_plate, const char *palm_sheet) {
  Renderer *r = calloc(1, sizeof(*r));
  if (!r)
    return NULL;
  r->light = jc_hybrid_light(12.0);
  r->hour = 12.0;
  r->environment_enabled = true;
  uint8_t rgba[256][4], lut[256][4];
  for (int i = 0; i < 256; ++i) {
    for (int j = 0; j < 3; ++j)
      rgba[i][j] = palette[i][j];
    rgba[i][3] = 255;
    jc_hybrid_color(palette[i], lut[i]);
  }
  r->atlas = sg_make_image(&(sg_image_desc){.width = JC_ATLAS_W,
                                            .height = JC_ATLAS_H,
                                            .pixel_format = SG_PIXELFORMAT_R8,
                                            .usage.copy_dst = true,
                                            .label = "owned indexed atlas"});
  r->palette =
      sg_make_image(&(sg_image_desc){.width = 256,
                                     .height = 1,
                                     .pixel_format = SG_PIXELFORMAT_RGBA8,
                                     .data.mip_levels[0] = {rgba, sizeof rgba},
                                     .label = "original palette"});
  r->atlas_view = sg_make_view(&(sg_view_desc){.texture.image = r->atlas});
  r->palette_view = sg_make_view(&(sg_view_desc){.texture.image = r->palette});
  r->staging =
      sg_make_buffer(&(sg_buffer_desc){.size = JC_RELAY_MIB * 1024 * 1024,
                                       .usage = {.staging_buffer = true,
                                                 .copy_src = true,
                                                 .write_transient = true},
                                       .label = "bounded atlas relay"});
  r->nearest =
      sg_make_sampler(&(sg_sampler_desc){.min_filter = SG_FILTER_NEAREST,
                                         .mag_filter = SG_FILTER_NEAREST,
                                         .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                                         .wrap_v = SG_WRAP_CLAMP_TO_EDGE});
  r->shader = sg_make_shader(indexed_shader_desc(sg_query_backend()));
  r->pipeline = sg_make_pipeline(
      &(sg_pipeline_desc){.shader = r->shader, .label = "indexed sprites"});
  r->hybrid_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = r->shader,
      .colors[0].blend = {.enabled = true,
                          .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                          .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                          .src_factor_alpha = SG_BLENDFACTOR_ONE,
                          .dst_factor_alpha =
                              SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA},
      .label = "hybrid R8, edges and projected shadows"});
  r->lut =
      sg_make_image(&(sg_image_desc){.width = 256,
                                     .height = 1,
                                     .pixel_format = SG_PIXELFORMAT_RGBA8,
                                     .data.mip_levels[0] = {lut, sizeof lut},
                                     .label = "harmonized runtime LUT"});
  r->lut_view = sg_make_view(&(sg_view_desc){.texture.image = r->lut});
  if (sg_query_buffer_state(r->staging) != SG_RESOURCESTATE_VALID ||
      sg_query_image_state(r->atlas) != SG_RESOURCESTATE_VALID ||
      sg_query_image_state(r->lut) != SG_RESOURCESTATE_VALID ||
      sg_query_pipeline_state(r->hybrid_pipeline) != SG_RESOURCESTATE_VALID ||
      sg_query_image_state(r->palette) != SG_RESOURCESTATE_VALID ||
      sg_query_pipeline_state(r->pipeline) != SG_RESOURCESTATE_VALID ||
      sg_query_view_state(r->atlas_view) != SG_RESOURCESTATE_VALID ||
      sg_query_view_state(r->palette_view) != SG_RESOURCESTATE_VALID ||
      sg_query_sampler_state(r->nearest) != SG_RESOURCESTATE_VALID) {
    renderer_destroy(r);
    return NULL;
  }
  if (modern_plate && !load_plate(r, modern_plate)) {
    renderer_destroy(r);
    return NULL;
  }
  fprintf(stderr, "Atlas relay accepted: %zu bytes\n",
          sg_query_buffer_desc(r->staging).size);
  if (palm_sheet && !load_palm(r, palm_sheet)) {
    renderer_destroy(r);
    return NULL;
  }
  return r;
}
static void draw_palm(Renderer *r, const JCDraw *d, const JCRenderFrame *f) {
  if (!r->palm_view.id) {
    /* Exact source geometry, including clipped redraws and alternate TTM
     * silhouettes. Never map a fixed painted crown onto an animated leaf. */
    draw_params_t p = {
        .rect = {jc_layout_x((float)d->x - f->origin_x),
                 jc_layout_y((float)d->y - f->origin_y),
                 jc_layout_width((float)d->w), jc_layout_width((float)d->h)},
        .canvas = {1280.f, 720.f, 0, 0},
        .uv_rect = {(float)d->ax / JC_ATLAS_W, (float)d->ay / JC_ATLAS_H,
                    (float)d->w / JC_ATLAS_W, (float)d->h / JC_ATLAS_H},
        .flags = {(d->flags & FLIP_H) ? 1.f : 0.f,
                  (d->flags & FLIP_V) ? 1.f : 0.f,
                  d->transparent ? 1.f : 0.f, 1.f}};
    memcpy(p.tint, r->light.tint, sizeof p.tint);
    memcpy(p.sun, r->light.sun, sizeof p.sun);
    memcpy(p.breeze, r->breeze, sizeof p.breeze);
    if (d->source_sprite != 12 || d->domain != GFX_SCENERY)
      p.breeze[1] = 0;
    memcpy(p.fire_light, r->fire, sizeof p.fire_light);
    p.decor[0] = r->environment_enabled ? 1.f : 0.f;
    if (d->source_sprite == 14 && d->domain == GFX_SCENERY) {
      /* The original ground silhouette is already flat: use the existing
       * soft contact-shadow material, rather than an opaque grey patch. */
      p.flags[3] = 2.f;
      p.projection[2] = 1.f;
      if (r->environment_enabled) {
        p.projection[2] = 2.f;
        p.projection[0] = r->light.skew * .45f;
        p.segment[1] = jc_layout_y(293.f);
      }
      p.material[0] = 1.f;
    }
    sg_bindings b = {0};
    b.views[VIEW_atlas] = r->atlas_view;
    b.views[VIEW_palette] = r->lut_view;
    b.views[VIEW_background_plate] = r->plate_view;
    b.views[VIEW_environment_mask] = r->environment_view;
    b.samplers[SMP_nearest_sampler] = r->nearest;
    b.samplers[SMP_linear_sampler] = r->linear;
    sg_apply_pipeline(r->hybrid_pipeline);
    sg_apply_bindings(&b);
    sg_apply_uniforms(UB_draw_params, &SG_RANGE(p));
    sg_draw(0, 6, 1);
    ++r->original_palm_draws;
    return;
  }
  JCDraw registered = jc_palm_draw(f, d);
  d = &registered;
  float mode = d->source_sprite == 12   ? 1.f
               : d->source_sprite == 14 ? 2.f
                                        : 0.f;
  foliage_params_t p = {
      .foliage_rect = {jc_layout_x((float)d->x - f->origin_x),
                       jc_layout_y((float)d->y - f->origin_y),
                       jc_layout_width((float)d->w),
                       jc_layout_width((float)d->h)},
      .mask_uv = {(float)d->ax / JC_ATLAS_W, (float)d->ay / JC_ATLAS_H,
                  (float)d->w / JC_ATLAS_W, (float)d->h / JC_ATLAS_H},
      .foliage_mode = {mode, (d->flags & FLIP_H) ? 1.f : 0.f,
                       (d->flags & FLIP_V) ? 1.f : 0.f, 0}};
  memcpy(p.foliage_tint, r->light.tint, sizeof p.foliage_tint);
  memcpy(p.foliage_sun, r->light.sun, sizeof p.foliage_sun);
  memcpy(p.foliage_breeze, r->breeze, sizeof p.foliage_breeze);
  memcpy(p.foliage_fire, r->fire, sizeof p.foliage_fire);
  if (mode == 1.f) {
    p.foliage_uv[0] = 40.f / 1024.f;
    p.foliage_uv[1] = 15.f / 1536.f;
    p.foliage_uv[2] = 950.f / 1024.f;
    p.foliage_uv[3] = 745.f / 1536.f;
  } else {
    p.foliage_uv[0] = 501.f / 1024.f;
    p.foliage_uv[1] = 780.f / 1536.f;
    p.foliage_uv[2] = 29.f / 1024.f;
    p.foliage_uv[3] = 600.f / 1536.f;
  }
  sg_apply_pipeline(r->foliage_pipeline);
  sg_bindings b = {0};
  b.views[VIEW_palm_sheet] = r->palm_view;
  b.views[VIEW_original_mask] = r->atlas_view;
  b.samplers[SMP_foliage_sampler] = r->linear;
  b.samplers[SMP_mask_sampler] = r->nearest;
  sg_apply_bindings(&b);
  sg_apply_uniforms(UB_foliage_params, &SG_RANGE(p));
  sg_draw(0, 6, 1);
}
bool renderer_frame(Renderer *r, const SceneState *s, sg_swapchain sc,
                    bool interpolate) {
  const JCRenderFrame *f = s->current;
  if (sc.invalid)
    return false;
  if (r->blocked && r->blocked_cut != f->cut_id)
    r->blocked = false;
  if (!f->modern_compatible) {
    r->blocked = true;
    r->blocked_cut = f->cut_id;
  }
  bool use_layers = r->modern_loaded && f->scenery_count == 3 && !r->blocked &&
                    f->modern_compatible;
  if (!r->uploaded || r->serial != f->serial) {
    const size_t length = (size_t)JC_ATLAS_W * f->height;
    uint8_t *prepared = NULL;
    const uint8_t *pixels = f->atlas;
    if (use_layers) {
      size_t bytes =
          length > (size_t)(JC_STREAM_MIB < JC_RELAY_MIB ? JC_STREAM_MIB
                                                         : JC_RELAY_MIB) *
                       1024 * 1024
              ? (size_t)JC_ATLAS_W * JC_ATLAS_H
              : length;
      prepared = malloc(bytes);
      if (!prepared)
        return false;
      memcpy(prepared, f->atlas, bytes);
      for (size_t i = 0; i < f->actor_count; ++i) {
        const JCDraw *d = &f->actors[i];
        r->body[i] = jc_sprite_body_bounds(f, d);
        r->johnny_material[i] = jc_hybrid_johnny(d);
        float x = jc_layout_x(d->x - (float)f->origin_x);
        float y = jc_layout_y(d->y - (float)f->origin_y);
        r->casts_shadow[i] =
            jc_hybrid_shadow(d, x, y, jc_layout_width((float)d->w),
                             jc_layout_width((float)r->body[i].bottom)) &&
            !jc_sprite_unhandled_shadow(f, d);
        /* Audited baked ground shadows must also disappear on clipped parts
         * and poses whose feet fall outside the sand/projection heuristic. */
        r->stripped_pixels += jc_sprite_strip_shadow(prepared, d);
      }
      pixels = prepared;
    }
    if (length > r->max_upload)
      r->max_upload = length;
    if (length >
        (size_t)(JC_STREAM_MIB < JC_RELAY_MIB ? JC_STREAM_MIB : JC_RELAY_MIB) *
            1024 * 1024) {
      sg_image replacement = sg_make_image(&(sg_image_desc){
          .width = JC_ATLAS_W,
          .height = JC_ATLAS_H,
          .pixel_format = SG_PIXELFORMAT_R8,
          .data.mip_levels[0] = {pixels, (size_t)JC_ATLAS_W * JC_ATLAS_H},
          .label = "large atlas bounded sequential copy"});
      if (sg_query_image_state(replacement) != SG_RESOURCESTATE_VALID) {
        free(prepared);
        return false;
      }
      sg_destroy_view(r->atlas_view);
      sg_destroy_image(r->atlas);
      r->atlas = replacement;
      r->atlas_view = sg_make_view(&(sg_view_desc){.texture.image = r->atlas});
      r->atlas_immutable = true;
    } else {
      if (r->atlas_immutable) {
        sg_image replacement =
            sg_make_image(&(sg_image_desc){.width = JC_ATLAS_W,
                                           .height = JC_ATLAS_H,
                                           .pixel_format = SG_PIXELFORMAT_R8,
                                           .usage.copy_dst = true,
                                           .label = "streamed indexed atlas"});
        if (sg_query_image_state(replacement) != SG_RESOURCESTATE_VALID) {
          free(prepared);
          return false;
        }
        sg_destroy_view(r->atlas_view);
        sg_destroy_image(r->atlas);
        r->atlas = replacement;
        r->atlas_view =
            sg_make_view(&(sg_view_desc){.texture.image = r->atlas});
        r->atlas_immutable = false;
      }
      sg_write_buffer_transient(&(sg_write_buffer_desc){
          .src.data = {pixels, length}, .dst.buffer = r->staging});
      sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
          .src = {.buffer = r->staging, .bytes_per_row = JC_ATLAS_W},
          .dst.image = r->atlas,
          .size = {.width = JC_ATLAS_W, .height = f->height, .num_slices = 1}});
    }
    free(prepared);
    r->uploaded = true;
    r->serial = f->serial;
  }
  sg_begin_pass(
      &(sg_pass){.swapchain = sc,
                 .action.colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                                      .clear_value = {0, 0, 0, 1}}});
  const bool modern = r->modern_loaded;
  const bool layered = modern && f->scenery_count == 3 && !r->blocked;
  r->breeze[0] = (float)fmod(r->environment_seconds, 4096.0);
  r->breeze[1] = layered && r->environment_enabled &&
                  jc_environment_wind(f->scene_id, f->walk_preview) ? 1.f : 0.f;
  r->breeze[2] = 79.f / 152.f;
  r->breeze[3] = 26.f / 69.f;
  memset(r->fire, 0, sizeof r->fire);
  if (layered && r->environment_enabled) {
    jc_environment_fire(f, r->fire);
    double phase = 6.283185307179586 * r->breeze[0] / 4096.0;
    r->fire[2] *= (float)(.91 + .06 * sin(phase * 5997) +
                         .03 * sin(phase * 15997));
  }
  if (layered && r->environment_enabled) {
    ++r->environment_frames;
    if (r->breeze[1] > 0)
      ++r->wind_frames;
    if (r->fire[2] > 0)
      ++r->fire_frames;
  }
  if (modern && !layered)
    ++r->composite_frames;
  if (r->modern_loaded && (!r->mode_known || r->was_modern != layered)) {
    fprintf(stderr, "Presentation: %s\n",
            layered ? "hybrid layered HD-2D" : "hybrid faithful composite");
    r->mode_known = true;
    r->was_modern = layered;
  }
  const float cw = layered ? 1280.f : WIDTH, ch = layered ? 720.f : HEIGHT;
  const float scale = fminf((float)sc.width / cw, (float)sc.height / ch);
  int w = (int)floorf(cw * scale), h = (int)floorf(ch * scale);
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
  const int vx = (sc.width - w) / 2, vy = (sc.height - h) / 2;
  sg_apply_viewport(vx, vy, w, h, true);
  if (layered) {
    sg_apply_scissor_rect(vx, vy, w, h, true);
    sg_apply_pipeline(r->modern_pipeline);
    sg_bindings mb = {0};
    mb.views[VIEW_background_plate] = r->plate_view;
    mb.views[VIEW_environment_mask] = r->environment_view;
    mb.samplers[SMP_linear_sampler] = r->linear;
    sg_apply_bindings(&mb);
    modern_params_t mp = {
        .clock = {r->environment_enabled ? r->breeze[0] :
                    (float)fmod(s->visual_seconds, 3600.0), 0,
                  r->environment_enabled ? 1.f : 0.f, 0}};
    mp.clock[1] = r->light.sun[2];
    memcpy(mp.plate_tint, r->light.tint, sizeof mp.plate_tint);
    JCAtmosphere atmosphere = jc_atmosphere(r->hour);
    memcpy(mp.sky_top, atmosphere.top, sizeof mp.sky_top);
    memcpy(mp.sky_horizon, atmosphere.horizon, sizeof mp.sky_horizon);
    memcpy(mp.sun, atmosphere.sun, sizeof mp.sun);
    memcpy(mp.moon, atmosphere.moon, sizeof mp.moon);
    memcpy(mp.environment, atmosphere.factors, sizeof mp.environment);
    memcpy(mp.campfire, r->fire, sizeof mp.campfire);
    sg_apply_uniforms(UB_modern_params, &SG_RANGE(mp));
    sg_draw(0, 3, 1);
    for (size_t i = 0; i < f->scenery_count; ++i)
      draw_palm(r, &f->scenery[i], f);
  }
  sg_apply_pipeline(modern ? r->hybrid_pipeline : r->pipeline);
  sg_bindings bindings = {0};
  bindings.views[VIEW_atlas] = r->atlas_view;
  bindings.views[VIEW_palette] = modern ? r->lut_view : r->palette_view;
  bindings.views[VIEW_background_plate] =
      r->plate_view.id ? r->plate_view : r->palette_view;
  bindings.views[VIEW_environment_mask] =
      r->environment_view.id ? r->environment_view : r->palette_view;
  bindings.samplers[SMP_linear_sampler] = r->linear.id ? r->linear : r->nearest;
  bindings.samplers[SMP_nearest_sampler] = r->nearest;
  sg_apply_bindings(&bindings);
  const size_t count = layered ? f->actor_count : f->count;
  for (int pass = 0; pass < (layered ? 2 : 1); ++pass) {
    bool shadow_only = layered && pass == 0;
    for (size_t i = 0; i < count; ++i) {
      const JCDraw *d = layered ? &f->actors[i] : &f->draws[i];
      bool authored_shadow = layered && jc_hybrid_authored_shadow(d);
      if (authored_shadow && !shadow_only)
        continue;
      float x, y;
      if (layered)
        jc_render_actor_position(f, s->previous, i, scene_alpha(s),
                                 interpolate && s->interpolate, &x, &y);
      else
        jc_render_frame_position(f, s->previous, i, scene_alpha(s),
                                 interpolate && s->interpolate, &x, &y);
      int x1 = d->clip.x1 < 0 ? 0 : d->clip.x1,
          y1 = d->clip.y1 < 0 ? 0 : d->clip.y1;
      int x2 = d->clip.x2 >= WIDTH ? WIDTH - 1 : d->clip.x2,
          y2 = d->clip.y2 >= HEIGHT ? HEIGHT - 1 : d->clip.y2;
      if (x2 < x1 || y2 < y1)
        continue;
      float dw = (float)d->w, dh = (float)d->h;
      uint8_t flags = d->flags;
      float cx1 = (float)x1, cy1 = (float)y1, cx2 = (float)x2 + 1,
            cy2 = (float)y2 + 1;
      if (layered) {
        x = jc_layout_x(x - f->origin_x);
        y = jc_layout_y(y - f->origin_y);
        dw = jc_layout_width(dw);
        dh = jc_layout_width(dh);
        cx1 = jc_layout_x(cx1 - f->origin_x);
        cx2 = jc_layout_x(cx2 - f->origin_x);
        cy1 = jc_layout_y(cy1 - f->origin_y);
        cy2 = jc_layout_y(cy2 - f->origin_y);
      }
      int sx = vx + (int)floorf(cx1 * w / cw),
          sy = vy + (int)floorf(cy1 * h / ch);
      int ex = vx + (int)ceilf(cx2 * w / cw),
          ey = vy + (int)ceilf(cy2 * h / ch);
      if (shadow_only)
        sg_apply_scissor_rect(vx, vy, w, h, true);
      else
        sg_apply_scissor_rect(sx, sy, ex - sx, ey - sy, true);
      if (layered && (d->domain == GFX_OCCLUDER ||
          (!strcmp(d->resource, "TRUNK.BMP") && d->source_sprite < 2))) {
        if (shadow_only)
          continue;
        if (d->domain == GFX_OCCLUDER && r->palm_view.id)
          sg_apply_scissor_rect(vx, vy, w, h, true);
        draw_palm(r, d, f);
        sg_apply_pipeline(modern ? r->hybrid_pipeline : r->pipeline);
        sg_apply_bindings(&bindings);
        continue;
      }
      float bottom = layered ? (float)r->body[i].bottom / d->h : 1.f;
      if (shadow_only && !r->casts_shadow[i] && !authored_shadow)
        continue;
      draw_params_t p = {
          .rect = {x, y, dw, dh},
          .canvas = {cw, ch, 0, 0},
          .uv_rect = {(float)d->ax / JC_ATLAS_W, (float)d->ay / JC_ATLAS_H,
                      (float)d->w / JC_ATLAS_W, (float)d->h / JC_ATLAS_H},
          .flags = {(flags & FLIP_H) ? 1.f : 0.f, (flags & FLIP_V) ? 1.f : 0.f,
                    d->transparent ? 1.f : 0.f,
                    modern ? (shadow_only ? 2.f : 1.f) : 0.f}};
      memcpy(p.tint, r->light.tint, sizeof p.tint);
      memcpy(p.sun, r->light.sun, sizeof p.sun);
      memcpy(p.fire_light, r->fire, sizeof p.fire_light);
      p.decor[0] = layered && r->environment_enabled ? 1.f : 0.f;
      p.projection[0] = r->light.skew;
      p.projection[1] = bottom;
      p.projection[2] = layered && jc_hybrid_contact(d, r->body[i]) ? 1.f : 0.f;
      p.material[0] = p.projection[2];
      p.material[1] =
          modern && (layered ? r->johnny_material[i] : jc_hybrid_johnny(d))
              ? 1.f
              : 0.f;
      if (authored_shadow) {
        /* Reuse the soft, translucent ground material at the authored position,
         * once in the shadow pass. Do not project this flat shape a second time. */
        p.projection[2] = 1.f;
        p.material[0] = 1.f;
        p.material[1] = 0.f;
      }
      p.material[3] = layered && r->environment_enabled &&
                      !strcmp(d->resource, "FIRE1.BMP") ? 1.f : 0.f;
      if (layered && d->line) {
        if (shadow_only)
          continue;
        p.flags[3] = 3.f;
        p.segment[0] = jc_layout_x(d->line_x1 - (float)f->origin_x);
        p.segment[1] = jc_layout_y(d->line_y1 - (float)f->origin_y);
        p.segment[2] = jc_layout_x(d->line_x2 - (float)f->origin_x);
        p.segment[3] = jc_layout_y(d->line_y2 - (float)f->origin_y);
        float pad = 2.f * cw / w;
        p.rect[0] = fminf(p.segment[0], p.segment[2]) - pad;
        p.rect[1] = fminf(p.segment[1], p.segment[3]) - pad;
        p.rect[2] = fabsf(p.segment[2] - p.segment[0]) + 2.f * pad;
        p.rect[3] = fabsf(p.segment[3] - p.segment[1]) + 2.f * pad;
        p.material[2] = d->line_color;
        ++r->line_draws;
      }
      sg_apply_uniforms(UB_draw_params, &SG_RANGE(p));
      sg_draw(0, 6, 1);
      if (shadow_only)
        ++r->shadow_draws;
      else if (modern) {
        ++r->fallback_draws;
        if (p.material[1] > .5f)
          ++r->johnny_draws;
      }
    }
  }
  sg_end_pass();
  return true;
}
void renderer_destroy(Renderer *r) {
  if (!r)
    return;
  sg_destroy_pipeline(r->foliage_pipeline);
  sg_destroy_shader(r->foliage_shader);
  sg_destroy_view(r->palm_view);
  sg_destroy_image(r->palm);
  sg_destroy_pipeline(r->modern_pipeline);
  sg_destroy_shader(r->modern_shader);
  sg_destroy_sampler(r->linear);
  sg_destroy_view(r->plate_view);
  sg_destroy_image(r->plate);
  sg_destroy_view(r->environment_view);
  sg_destroy_image(r->environment_mask);
  fprintf(
      stderr,
      "Hybrid stats: 0 Johnny RGBA sheets, %zu max R8 upload bytes, %llu R8 "
      "draws, %llu silhouette shadows, %llu faithful composite frames\n",
      r->max_upload, (unsigned long long)r->fallback_draws,
      (unsigned long long)r->shadow_draws,
      (unsigned long long)r->composite_frames);
  fprintf(stderr,
          "Uniform R8: %llu native-resolution line draws, %llu baked-shadow "
          "pixels removed; Johnny original only\n",
          (unsigned long long)r->line_draws,
          (unsigned long long)r->stripped_pixels);
  fprintf(stderr, "Johnny: %llu warm R8 draws; RGBA replacements disabled\n",
          (unsigned long long)r->johnny_draws);
  fprintf(stderr, "Palm: %llu original R8 draws\n",
          (unsigned long long)r->original_palm_draws);
  fprintf(stderr, "Decor: %llu live frames, %llu wind frames, %llu fire-lit "
                  "frames; mask %dx%d\n",
          (unsigned long long)r->environment_frames,
          (unsigned long long)r->wind_frames,
          (unsigned long long)r->fire_frames, JC_ENV_W, JC_ENV_H);
  sg_destroy_pipeline(r->hybrid_pipeline);
  sg_destroy_view(r->lut_view);
  sg_destroy_image(r->lut);
  sg_destroy_pipeline(r->pipeline);
  sg_destroy_shader(r->shader);
  sg_destroy_sampler(r->nearest);
  sg_destroy_view(r->atlas_view);
  sg_destroy_view(r->palette_view);
  sg_destroy_image(r->atlas);
  sg_destroy_image(r->palette);
  free(r);
}
