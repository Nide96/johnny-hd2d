/* C99 adaptation of open-scrantic engine/mod.rs, MIT (Glauber Lima).
 * Story: seg4; tick: seg2:16ce. Original assets/tables always loaded at
 * runtime. */
#include "engine.h"
#include "ads.h"
#include "island.h"
#include "walk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_PLAN 300
#define NO_SCENE SIZE_MAX
struct JCEngine {
  const JCAssets *data;
  JCConfig config;
  Rng rng;
  Island island;
  Walk walk;
  AdsRuntime ads;
  bool has_ads, walk_preview;
  unsigned preview_path;
  JCBmp johnwalk;
  uint8_t palette[256][3];
  Canvas bg, work, front, base;
  uint32_t now, idle_timer;
  int32_t offset_x, offset_y;
  size_t plan[MAX_PLAN], plan_len, plan_idx;
  bool *pending;
  size_t last_final, last_first, story_scene, forced_scene;
  int32_t story_state;
  bool new_session;
  uint64_t scenes_done, epoch;
  uint32_t sounds[128];
  size_t sound_count;
  JCWorldView presented_view;
  GfxTrace trace;
  void *trace_user;
};
JCConfig jc_config_default(void) {
  JCConfig c = {0};
  c.background = c.clouds = c.waves = c.sounds = true;
  c.start_time = 900;
  return c;
}
static bool fail(JCError *e, const char *s) {
  if (e)
    snprintf(e->message, sizeof e->message, "%s", s);
  return false;
}
JCEngine *jc_engine_create(const JCAssets *d, const JCConfig *config,
                           uint64_t seed, JCError *err) {
  JCEngine *e;
  JCPicture intro = {0};
  if (!d || !d->exe.scene_count) {
    fail(err, "empty scene catalog");
    return NULL;
  }
  e = calloc(1, sizeof *e);
  if (!e) {
    fail(err, "engine allocation failed");
    return NULL;
  }
  e->data = d;
  e->config = config ? *config : jc_config_default();
  e->last_final = e->last_first = e->story_scene = e->forced_scene = NO_SCENE;
  e->pending = malloc(d->exe.scene_count * sizeof *e->pending);
  if (!e->pending) {
    fail(err, "pending allocation failed");
    goto bad;
  }
  memset(e->pending, 1, d->exe.scene_count * sizeof *e->pending);
  if (!jc_archive_palette(&d->archive, "JOHNCAST.PAL", e->palette, err) ||
      !jc_archive_bmp(&d->archive, "JOHNWALK.BMP", &e->johnwalk, err) ||
      !island_create(&e->island, &d->archive, &d->exe, err) ||
      !jc_archive_scr(&d->archive, "INTRO.SCR", &intro, err))
    goto bad;
  rng_init(&e->rng, d->exe.rng_table, d->exe.rng_j, d->exe.rng_k);
  rng_warm_up(&e->rng, seed);
  walk_init(&e->walk);
  canvas_load_picture(&e->front, &intro);
  memcpy(e->bg.px, e->front.px, sizeof e->bg.px);
  memcpy(e->work.px, e->front.px, sizeof e->work.px);
  jc_picture_free(&intro);
  return e;
bad:
  jc_picture_free(&intro);
  jc_engine_destroy(e);
  return NULL;
}
void jc_engine_destroy(JCEngine *e) {
  if (!e)
    return;
  ads_free(&e->ads);
  island_free(&e->island);
  jc_bmp_free(&e->johnwalk);
  free(e->pending);
  free(e);
}
void jc_engine_tick(JCEngine *e) {
  ++e->now;
  if (e->walk.timer)
    --e->walk.timer;
  if (e->idle_timer)
    --e->idle_timer;
  if (e->island.wave_timer)
    --e->island.wave_timer;
  if (e->island.cap_timer)
    --e->island.cap_timer;
}
/* seg4:0c48 */
static int32_t half_hour(int32_t t) {
  return (t / 50 + (t % 100) / 30 + 14) % 16;
}
static void update_day(JCEngine *e, const JCClock *clock) {
  JCConfig *c = &e->config;
  if (clock->year != c->year || clock->month != c->month ||
      clock->day != c->day) {
    if (c->num_days < c->intro)
      ++c->num_days;
    if (c->num_days > 11 || c->num_days < 2)
      c->intro = c->num_days = 1;
    c->year = clock->year;
    c->month = clock->month;
    c->day = clock->day;
    c->intro_saved = 0;
  }
  if (e->new_session && e->story_state && c->intro <= c->num_days) {
    ++c->intro;
    c->intro_saved = c->intro;
  }
  e->story_state = 0;
  e->new_session = false;
}
static bool marked(const JCEngine *e, const JCSceneDef *s, uint32_t mask,
                   int32_t phase, int32_t budget) {
  if (mask == 0x80 && s->day == e->config.intro)
    return true;
  return (s->flags & mask) && s->ads_tag && s->min_phase <= phase && !s->day &&
         s->max_phase > phase && s->duration / 2 < budget &&
         island_compatible(&e->island, s->flags);
}
/* seg4:0d76 */
static size_t pick(JCEngine *e, uint32_t mask, int32_t phase, int32_t budget) {
  size_t i;
  int32_t total = 0, draw;
  if (mask == 4)
    budget = 999;
  else if (mask == 0x80) {
    if (rng_below(&e->rng, 10))
      return NO_SCENE;
    e->config.intro = e->config.num_days;
    budget = 999;
  }
  for (i = 0; i < e->data->exe.scene_count; ++i)
    if (marked(e, &e->data->exe.scenes[i], mask, phase, budget))
      total += e->data->exe.scenes[i].weight;
  if (!total)
    return NO_SCENE;
  draw = rng_below(&e->rng, (uint16_t)total) + 1;
  for (i = 0; i < e->data->exe.scene_count; ++i)
    if (marked(e, &e->data->exe.scenes[i], mask, phase, budget)) {
      draw -= e->data->exe.scenes[i].weight;
      if (draw <= 0)
        return i;
    }
  return NO_SCENE;
}
static uint32_t flags(const JCEngine *e, size_t i) {
  return e->data->exe.scenes[i].flags;
}
static int32_t duration(const JCEngine *e, size_t i) {
  return e->data->exe.scenes[i].duration;
}
static void append(JCEngine *e, size_t s) {
  if (e->plan_len < MAX_PLAN)
    e->plan[e->plan_len++] = s;
}
static void plan_opening(JCEngine *e, int32_t phase, int32_t *budget) {
  size_t s = pick(e, 1, phase, *budget);
  if (s != NO_SCENE) {
    append(e, s);
    *budget -= duration(e, s);
    island_apply_flags(&e->island, flags(e, s));
  }
}
/* seg4:08b9, bounded final draw: retained from upstream to avoid hangs. */
static void plan_session(JCEngine *e, int32_t phase, int32_t budget) {
  size_t first = NO_SCENE, fin = NO_SCENE, s, pending_first, i;
  unsigned attempts = 0;
  static const uint16_t repeats[] = {10, 20, 30, 20, 10, 10};
  e->plan_len = 0;
  if (e->forced_scene != NO_SCENE) {
    s = e->forced_scene;
    e->forced_scene = NO_SCENE;
    island_apply_flags(&e->island, flags(e, s));
    e->pending[s] = true;
    append(e, s);
    return;
  }
  s = pick(e, 0x80, phase, 0);
  if (s != NO_SCENE && s != e->last_final && s != e->last_first) {
    if (flags(e, s) & 4)
      fin = s;
    else
      first = s;
    e->story_scene = s;
  }
  for (;;) {
    if (fin != NO_SCENE && fin != e->last_final &&
        !(first != NO_SCENE && (flags(e, fin) & 1)))
      break;
    ++attempts;
    fin = attempts < 1000 ? pick(e, 4, phase, budget) : NO_SCENE;
    if (fin == NO_SCENE) {
      for (i = 0; i < e->data->exe.scene_count; ++i)
        if (flags(e, i) & 4) {
          fin = i;
          break;
        }
      if (fin == NO_SCENE)
        fin = 0;
      break;
    }
  }
  budget -= duration(e, fin);
  island_apply_flags(&e->island, flags(e, fin));
  e->last_final = fin;
  if (first != NO_SCENE) {
    budget -= duration(e, first);
    island_apply_flags(&e->island, flags(e, first));
  }
  e->last_first = first;
  if (e->data->exe.scenes[fin].day == e->config.intro ||
      (first != NO_SCENE && e->data->exe.scenes[first].day == e->config.intro))
    e->story_state = 1;
  pending_first = first;
  if (flags(e, fin) & 1)
    budget = 0;
  else if (pending_first != NO_SCENE && (flags(e, pending_first) & 1)) {
    if (rng_chance(&e->rng, 50)) {
      append(e, pending_first);
      pending_first = NO_SCENE;
    } else
      plan_opening(e, phase, &budget);
  } else
    plan_opening(e, phase, &budget);
  while (budget != 0 && e->plan_len < 298) {
    int reps = 1;
    if (pending_first != NO_SCENE && rng_chance(&e->rng, 5)) {
      append(e, pending_first);
      pending_first = NO_SCENE;
      continue;
    }
    s = pick(e, 2, phase, budget);
    if (s == NO_SCENE)
      break;
    if (flags(e, s) & 8) {
      unsigned guard = 0;
      do {
        reps = (int)rng_pick(&e->rng, repeats, 6);
        if (++guard > 10000) {
          reps = 1;
          break;
        }
      } while (!reps || (reps > 1 && duration(e, s) * reps > budget));
    }
    while (reps > 0 && e->plan_len < 298) {
      append(e, s);
      budget -= duration(e, s);
      island_apply_flags(&e->island, flags(e, s));
      --reps;
    }
  }
  append(e, fin);
  for (i = 0; i < e->plan_len; ++i)
    e->pending[e->plan[i]] = true;
}
static bool new_session(JCEngine *e, const JCClock *clock, JCError *err) {
  ++e->epoch;
  const JCSceneDef *first;
  IslandOptions opts;
  int32_t t = clock->hour * 100 + (clock->minute >= 30 ? 30 : 0),
          phase = (half_hour(t) - half_hour(e->config.start_time) + 16) % 16;
  ads_free(&e->ads);
  e->has_ads = false;
  e->island.phase = phase;
  e->island.holiday = 0;
  if (clock->month == 10 && clock->day >= 29)
    e->island.holiday = 1;
  else if (clock->month == 12 && clock->day >= 23 && clock->day <= 25)
    e->island.holiday = 2;
  else if ((clock->month == 12 && clock->day >= 29) ||
           (clock->month == 1 && clock->day == 1))
    e->island.holiday = 3;
  else if (clock->month == 3 && clock->day >= 15 && clock->day <= 17)
    e->island.holiday = 4;
  island_reset_config(&e->island);
  update_day(e, clock);
  plan_session(e, phase, 300);
  if (!e->plan_len)
    return fail(err, "empty session plan");
  first = &e->data->exe.scenes[e->plan[0]];
  e->walk.spot = first->spot_start;
  e->walk.hdg = first->hdg_start;
  e->plan_idx = 0;
  opts =
      (IslandOptions){e->config.background, e->config.clouds, e->config.waves};
  if (!island_init(&e->island, &e->data->archive, &e->bg, &e->rng, &opts,
                   e->config.num_days, err))
    return false;
  island_ttm_offset(&e->island, false, &e->offset_x, &e->offset_y);
  return true;
}
static bool load_ads(JCEngine *e, uint8_t id, JCError *err) {
  const char *name;
  if (e->has_ads && e->ads.id == id)
    return true;
  ++e->epoch;
  ads_free(&e->ads);
  e->has_ads = false;
  name = e->data->exe.ads_names[id];
  if (!name)
    return fail(err, "scene references unknown ADS ID");
  if (!ads_load(&e->ads, &e->data->archive, id, name))
    return fail(err, e->ads.error);
  e->has_ads = true;
  return true;
}
static void complete(JCEngine *e, size_t si) {
  const JCSceneDef *s = &e->data->exe.scenes[si];
  e->walk.spot = s->spot_end;
  e->walk.hdg = s->hdg_end;
  e->pending[si] = true;
  ++e->plan_idx;
  ++e->scenes_done;
}
static void sound(void *p, uint32_t id) {
  JCEngine *e = p;
  if (e->sound_count < sizeof e->sounds / sizeof *e->sounds)
    e->sounds[e->sound_count++] = id;
}
/* seg4:06bf: complete background + ambient + walk/ADS + holiday composition. */
bool jc_engine_step(JCEngine *e, const JCClock *clock, JCError *err) {
  size_t si;
  const JCSceneDef *d;
  bool arrived;
  e->sound_count = 0;
  if (e->plan_idx >= e->plan_len) {
    e->new_session = true;
    if (!new_session(e, clock, err))
      return false;
  }
  memcpy(e->base.px, e->bg.px, sizeof e->base.px);
  memcpy(e->work.px, e->bg.px, sizeof e->work.px);
  e->work.trace = e->trace;
  e->work.trace_user = e->trace_user;
  e->work.trace_key = UINT64_C(0x8000000000000000);
  si = e->plan[e->plan_idx];
  d = &e->data->exe.scenes[si];
  e->presented_view =
      (JCWorldView){e->island.x, e->island.y,
                    e->island.mode == ISLAND_NORMAL && e->island.phase < 12 &&
                        !(d->flags & 0x400),
                    e->walk_preview, (unsigned)si};
  island_trace_layout(&e->island, e->trace, e->trace_user);
  e->work.trace_domain = GFX_AMBIENT;
  island_step(&e->island, &e->work, &e->rng);
  e->work.trace_domain = GFX_ACTOR;
  if (e->walk_preview) {
    Walk *w = &e->walk;
    if (!w->has_path && !w->timer) {
      unsigned path = e->preview_path;
      w->has_route = false;
      w->has_path = true;
      w->path_id = path;
      w->path_idx = 0;
      w->hdg = w->path_start_hdg = e->data->exe.path_info[path][0];
      w->path_end_hdg = e->data->exe.path_info[path][1];
      w->target = w->path_end_spot = e->data->exe.path_info[path][2];
      w->target_hdg = -1;
      e->preview_path = path == 25 ? 19 : 25;
    }
    e->work.trace_key = UINT64_C(0x8100000000000000);
    walk_step(w, &e->data->exe, &e->rng, &e->johnwalk, &e->island, &e->work);
    memcpy(e->front.px, e->work.px, sizeof e->front.px);
    return true;
  }
  arrived =
      !e->pending[si] || walk_go_to(&e->walk, d->spot_start, d->hdg_start);
  if (!arrived) {
    e->work.trace_key = UINT64_C(0x8100000000000000);
    walk_step(&e->walk, &e->data->exe, &e->rng, &e->johnwalk, &e->island,
              &e->work);
  } else if (d->ads_id == 0xff) {
    if (e->pending[si]) {
      e->pending[si] = false;
      e->idle_timer = rng_below(&e->rng, 240) + 180;
      walk_reset(&e->walk);
    }
    e->work.trace_key = UINT64_C(0x8100000000000000);
    walk_step(&e->walk, &e->data->exe, &e->rng, &e->johnwalk, &e->island,
              &e->work);
    if (!e->idle_timer)
      complete(e, si);
  } else {
    VmEnv env = {0};
    bool busy;
    if (e->pending[si]) {
      if (!load_ads(e, d->ads_id, err))
        return false;
      ads_start_tag(&e->ads, d->ads_tag);
      e->pending[si] = false;
      island_ttm_offset(&e->island, (d->flags & 0x400) != 0, &e->offset_x,
                        &e->offset_y);
    }
    env.work = &e->work;
    env.bg = &e->bg;
    env.front = &e->front;
    env.archive = &e->data->archive;
    env.johnwalk = &e->johnwalk;
    env.rng = &e->rng;
    env.sound = sound;
    env.sound_user = e;
    env.offset_x = e->offset_x;
    env.offset_y = e->offset_y;
    env.now = e->now;
    (void)ads_update(&e->ads, &env);
    if (env.failed)
      return fail(err, env.error);
    if (e->ads.failed)
      return fail(err, e->ads.error);
    busy = ads_tag_busy(&e->ads, d->ads_tag);
    if (!busy) {
      complete(e, si);
      if (!e->walk.spot)
        e->walk.spot = rng_below(&e->rng, 6) + 1;
      if (!e->walk.hdg)
        e->walk.hdg = rng_next(&e->rng) & 7;
      if (e->story_state == 2) {
        ++e->config.intro;
        e->config.intro_saved = e->config.intro;
        e->story_state = 3;
      }
    }
  }
  e->work.trace_key = UINT64_C(0x8200000000000000);
  e->work.trace_domain = GFX_HOLIDAY;
  island_draw_holiday(&e->island, &e->work);
  if (e->story_scene == si) {
    sound(e, 25);
    e->story_scene = NO_SCENE;
    e->story_state = 2;
  }
  /* Work is rebuilt before the next step; copying only the presented pixels
   * preserves canvas trace metadata and avoids a 300 KiB stack swap. */
  memcpy(e->front.px, e->work.px, sizeof e->front.px);
  return true;
}
const Canvas *jc_engine_frame(const JCEngine *e) { return &e->front; }
const Canvas *jc_engine_base(const JCEngine *e) { return &e->base; }
const uint8_t (*jc_engine_palette(const JCEngine *e))[3] { return e->palette; }
const JCConfig *jc_engine_config(const JCEngine *e) { return &e->config; }
uint64_t jc_engine_scenes_done(const JCEngine *e) { return e->scenes_done; }
uint64_t jc_engine_epoch(const JCEngine *e) { return e->epoch; }
uint32_t jc_engine_now(const JCEngine *e) { return e->now; }
size_t jc_engine_scene_count(const JCEngine *e) {
  return e->data->exe.scene_count;
}
bool jc_engine_force_scene(JCEngine *e, size_t i) {
  if (i >= e->data->exe.scene_count)
    return false;
  e->forced_scene = i;
  e->plan_idx = e->plan_len;
  return true;
}
const uint32_t *jc_engine_sounds(const JCEngine *e, size_t *n) {
  *n = e->sound_count;
  return e->sounds;
}
void jc_engine_set_trace(JCEngine *e, GfxTrace f, void *p) {
  e->trace = f;
  e->trace_user = p;
}

JCWorldView jc_engine_world_view(const JCEngine *e) {
  return e->presented_view;
}

void jc_engine_walk_preview(JCEngine *e) {
  e->walk_preview = true;
  e->preview_path = 25;
  walk_init(&e->walk);
}
