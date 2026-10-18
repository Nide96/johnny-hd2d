/* C99 adaptation of island.rs, MIT open-scrantic (Glauber Lima).
 * Original SCRANTIC seg3 addresses retained at semantic entry points. */
#include "island.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
  int32_t x, y, w, h;
} Rect;
typedef struct {
  Rect r;
  size_t first, counter;
} Wave;
static const Wave high_waves[] = {{{0x10f, 0x132, 0x48, 0x1d}, 3, 0},
                                  {{0x16d, 0x13f, 0x9c, 0x19}, 6, 1},
                                  {{0x207, 0x12f, 0x41, 0x20}, 9, 2}};
static const Wave low_waves[] = {{{0xea, 0x143, 0x78, 0x30}, 30, 0},
                                 {{0x170, 0x164, 0xab, 0x1c}, 33, 1},
                                 {{0x22f, 0x143, 0x51, 0x30}, 36, 2},
                                 {{0x82, 0x154, 0x61, 0x1d}, 39, 0}};
static const Rect trunk = {0x1bb, 0x94, 0x16, 0x91},
                  leaves = {0x16e, 0x7a, 0x94, 0x45},
                  shadow = {0x18d, 0x117, 0x68, 0x26},
                  island = {0x121, 0x117, 0x117, 0x33},
                  low_shore = {0xfa, 0x12f, 0x17c, 0x45},
                  rock = {0x97, 0x148, 0x39, 0x1e};
static int overlap(Rect a, Rect b) {
  bool inside = a.x >= b.x && a.x + a.w < b.x + b.w && a.y >= b.y &&
                a.y + a.h < b.y + b.h;
  bool hit = a.x + a.w >= b.x && b.x + b.w > a.x && a.y + a.h >= b.y &&
             b.y + b.h > a.y;
  return inside + hit;
}
static Rect rect(const Island *i, Rect r) {
  r.x += i->x;
  r.y += i->y;
  return r;
}
static bool empty(IslandSlot s) { return s.x == 0 && s.y == 0; }
static bool slot_tick(IslandSlot *s) {
  bool expired = s->counter == 0;
  --s->counter;
  return expired;
}
static IslandSlot slot(int32_t x, int32_t y, int32_t kind, int32_t interval,
                       int32_t counter) {
  IslandSlot s = {x, y, 0, kind, interval, counter};
  return s;
}
static void rand_pos(Rng *r, const JCSprite *s, Rect a, int32_t *x,
                     int32_t *y) {
  if (s) {
    a.w -= s->w;
    a.h -= s->h;
  }
  if (a.w < 0 || a.h < 0) {
    *x = *y = 0;
    return;
  }
  *x = a.x + (a.w ? rng_below(r, (uint16_t)a.w) : 0);
  *y = a.y + (a.h ? rng_below(r, (uint16_t)a.h) : 0);
}
static void sprite(const Island *i, Canvas *c, size_t idx, int32_t x, int32_t y,
                   uint8_t flags, Clip clip) {
  if (idx < i->background.count)
    canvas_sprite(c, &i->background.sprites[idx], i->x + x, i->y + y, flags,
                  clip);
}
bool island_create(Island *i, const JCArchive *a, const JCExe *e,
                   JCError *err) {
  memset(i, 0, sizeof *i);
  i->exe = e;
  i->cfg = i->tide = -1;
  i->holiday_ok = true;
  i->wind = 1;
  i->waves[0] = i->waves[1] = i->waves[2] = 2;
  return jc_archive_bmp(a, "BACKGRND.BMP", &i->background, err) &&
         jc_archive_bmp(a, "MRAFT.BMP", &i->raft, err) &&
         jc_archive_bmp(a, "HOLIDAY.BMP", &i->holiday_bmp, err);
}
void island_free(Island *i) {
  jc_bmp_free(&i->background);
  jc_bmp_free(&i->raft);
  jc_bmp_free(&i->holiday_bmp);
}
void island_reset_config(Island *i) {
  i->cfg = i->tide = -1;
  i->freeze_waves = false;
  i->mode = ISLAND_NORMAL;
  i->no_raft = false;
  i->holiday_ok = true;
}
/* seg3:1c41 */
void island_apply_flags(Island *i, uint32_t flags) {
  uint32_t l = flags & 65535, h = flags >> 16;
  if (l & 0x200)
    i->tide = 0;
  if (l & 0x4000)
    i->no_raft = true;
  if (l & 0x10)
    i->holiday_ok = false;
  if (l & 0x1000)
    i->freeze_waves = true;
  if (l & 0x800)
    i->mode = ISLAND_NIGHT;
  if (l & 0x2000)
    i->mode = ISLAND_NONE;
  if (i->cfg == -1 && (h & 0x3f))
    i->cfg = 0;
  if (h & 0x20)
    i->cfg = 1;
  else if (h & 0x10)
    i->cfg = 2;
  if (h & 1)
    i->cfg |= 4;
  if (h & 2)
    i->cfg |= 8;
  if (h & 4)
    i->cfg |= 0x10;
  if (h & 8)
    i->cfg |= 0x20;
}
/* seg4:0edb */
bool island_compatible(const Island *i, uint32_t f) {
  uint32_t h = f >> 16;
  int c = i->cfg;
  if (c == -1)
    return true;
  return !(((h & 0x20) && (c & 0x2a)) || ((h & 0x10) && (c & 0x15)) ||
           ((h & 1) && !(h & 2) && (c & 0x2a)) ||
           ((h & 1) && !(h & 4) && (c & 0x30)) ||
           ((h & 2) && !(h & 1) && (c & 0x15)) ||
           ((h & 2) && !(h & 8) && (c & 0x30)) ||
           ((h & 4) && !(h & 8) && (c & 0x2a)) ||
           ((h & 4) && !(h & 1) && (c & 0xc)) ||
           ((h & 8) && !(h & 4) && (c & 0x15)) ||
           ((h & 8) && !(h & 2) && (c & 0xc)));
}
void island_ttm_offset(const Island *i, bool shifted, int32_t *x, int32_t *y) {
  *x = i->mode == ISLAND_NONE ? 0 : i->x + 1 + (shifted ? 0x110 : 0);
  *y = i->mode == ISLAND_NONE ? 0 : i->y;
}
static void place(Island *i, Rng *r) {
  int32_t x, y, c;
  Rect a;
  if (i->cfg == -1)
    i->cfg = 0;
  c = i->cfg;
  if (c & 2) {
    x = 0x121;
    y = 0x12b;
  } else if (c & 1) {
    x = 0x11;
    y = 0x135;
  } else {
    a = i->tide == 1 ? (Rect){0xa8, 0xff, 0x6f, 0x3b}
                     : (Rect){0x43, 0xff, 0xe2, 0x81};
    if (c) {
      if (c == 0x10 || c == 0x20 || c == 0x30)
        a.y += a.h / 2;
      if (!((c & 4 && c & 0x10) || (c & 8 && c & 0x20)))
        a.h /= 2;
      if (c == 8 || c == 0x20 || c == 0x28)
        a.x += a.w / 2;
      if (!((c & 4 && c & 8) || (c & 0x10 && c & 0x20)))
        a.w /= 2;
    }
    rand_pos(r, NULL, a, &x, &y);
  }
  i->x = x - 0x121;
  i->y = y - 0x117;
}
static const Wave *wave_spots(const Island *i, size_t *count) {
  *count = i->tide == 1 ? 4 : 3;
  return i->tide == 1 ? low_waves : high_waves;
}
static void update_waves(Island *i) {
  size_t n;
  if (i->wave_timer)
    return;
  i->wave_timer = 4;
  if (i->waves[0] == i->waves[2])
    ++i->waves[0];
  else if (i->waves[1] != i->waves[0])
    ++i->waves[1];
  else
    ++i->waves[2];
  for (n = 0; n < 3; ++n)
    if (i->waves[n] > 2)
      i->waves[n] = 0;
}
static void draw_wave(const Island *i, Canvas *c, Wave w, Clip clip) {
  sprite(i, c, w.first + (size_t)i->waves[w.counter], w.r.x, w.r.y, 0, clip);
}
static void draw_waves(const Island *i, Canvas *c) {
  size_t k, n;
  const Wave *w = wave_spots(i, &n);
  for (k = 0; k < n; ++k)
    draw_wave(i, c, w[k], CLIP_FULL);
}
static void draw_raft(const Island *i, Canvas *c) {
  int32_t x, y;
  GfxDomain saved;
  if (i->no_raft || !i->raft_stage)
    return;
  x = i->tide == 0 ? 0x201 : 0x212;
  y = i->tide == 0 ? 0x10a : 0x119;
  saved = c->trace_domain;
  c->trace_domain = GFX_RAFT;
  if ((size_t)(i->raft_stage - 1) < i->raft.count)
    canvas_sprite(c, &i->raft.sprites[i->raft_stage - 1], i->x + x, i->y + y, 0,
                  CLIP_FULL);
  c->trace_domain = saved;
}
void island_draw_holiday(const Island *i, Canvas *c) {
  size_t k;
  int x, y;
  if (!i->holiday || !i->holiday_ok || i->mode == ISLAND_NONE)
    return;
  switch (i->holiday) {
  case 1:
    k = 0;
    x = 0x19b;
    y = 0x12a;
    break;
  case 2:
    k = 2;
    x = 0x195;
    y = 0x10b;
    break;
  case 3:
    k = 3;
    x = 0x16a;
    y = 0x9b;
    break;
  default:
    k = 1;
    x = 0x14e;
    y = 0x11e;
    break;
  }
  if (k < i->holiday_bmp.count)
    canvas_sprite(c, &i->holiday_bmp.sprites[k], i->x + x, i->y + y, 0,
                  CLIP_FULL);
}
void island_trunk_overlay(const Island *i, Canvas *c, int32_t x, int32_t y,
                          int32_t w, int32_t h) {
  Rect r = {x, y, w, h};
  GfxDomain saved = c->trace_domain;
  c->trace_domain = GFX_OCCLUDER;
  if (y + h < i->y + 0x125 && overlap(r, rect(i, trunk)))
    sprite(i, c, 13, trunk.x, trunk.y, 0, clip_rect(x, y, w, h));
  c->trace_domain = saved;
}
static void part(const Island *i, Canvas *c, Rect r, size_t idx, Rect p,
                 Clip clip) {
  if (overlap(r, rect(i, p)))
    sprite(i, c, idx, p.x, p.y, 0, clip);
}
static void redraw_over(const Island *i, Canvas *c, Rect r) {
  Clip clip = clip_rect(r.x, r.y, r.w, r.h);
  Rect bottom = {r.x, r.y + r.h, r.w, 1};
  size_t n;
  if (overlap(r, (Rect){0, i->y + 0xd6, WIDTH, 0x10a})) {
    if (i->tide == 1) {
      part(i, c, r, 2, rock, clip);
      if (overlap(bottom, rect(i, low_waves[3].r)))
        draw_wave(i, c, low_waves[3], clip);
      part(i, c, r, 0, island, clip);
      part(i, c, r, 1, low_shore, clip);
      for (n = 0; n < 3; ++n)
        if (overlap(bottom, rect(i, low_waves[n].r)))
          draw_wave(i, c, low_waves[n], clip);
    } else {
      part(i, c, r, 0, island, clip);
      for (n = 0; n < 3; ++n)
        if (overlap(bottom, rect(i, high_waves[n].r)))
          draw_wave(i, c, high_waves[n], clip);
    }
  }
  part(i, c, r, 13, trunk, clip);
  part(i, c, r, 12, leaves, clip);
  part(i, c, r, 14, shadow, clip);
}
static bool init_clouds(Island *i, Canvas *bg, Rng *r, size_t count) {
  int32_t quota[3] = {8, 4, 2};
  IslandSlot cloud[15] = {{0}};
  Canvas *work;
  size_t n;
  uint16_t w50[] = {50}, wfirst[] = {5, 10, 15}, wsecond[] = {20, 30, 50};
  i->wind = rng_pick(r, w50, 1) == 1 ? -1 : 1;
  for (n = 0; n < count; ++n) {
    int k, pick, interval;
    const JCSprite *s;
    int32_t x, y;
    if (!rng_chance(r, 30))
      continue;
    pick = (int)rng_pick(r, wfirst, 3);
    k = pick == 1 ? 2 : pick == 2 ? 1 : pick == 3 ? 0 : -1;
    while (k >= 0 && !quota[k])
      --k;
    if (k < 0)
      continue;
    --quota[k];
    s = (size_t)(15 + k) < i->background.count ? &i->background.sprites[15 + k]
                                               : NULL;
    rand_pos(r, s, (Rect){0, 1, WIDTH, 0x8b}, &x, &y);
    interval = rng_below(r, 5) + 6;
    cloud[n] = slot(x, y, k, interval, rng_below(r, (uint16_t)interval));
  }
  for (n = 0; n < count; ++n) {
    int pick, k;
    int32_t x, y;
    const JCSprite *s;
    if (!empty(cloud[n]))
      continue;
    pick = (int)rng_pick(r, wsecond, 3);
    k = pick == 1 ? 2 : pick == 2 ? 1 : pick == 3 ? 0 : -1;
    if (k < 0 || !quota[k])
      continue;
    --quota[k];
    s = (size_t)(15 + k) < i->background.count ? &i->background.sprites[15 + k]
                                               : NULL;
    rand_pos(r, s, (Rect){i->wind == 1 ? -1000 : WIDTH, 1, 1000, 0x8b}, &x, &y);
    cloud[n] = slot(x, y, k, rng_below(r, 5) + 6, 0);
  }
  /* Clouds only bake once: full image staging lives on heap to keep stack
   * bounded. */
  work = malloc(sizeof *work);
  if (!work)
    return false;
  memset(work, 0, sizeof *work);
  memcpy(work->px, bg->px, sizeof work->px);
  for (n = 0; n < count; ++n) {
    IslandSlot *s = &cloud[n];
    const JCSprite *p;
    if (empty(*s))
      continue;
    if (slot_tick(s)) {
      s->counter = s->interval;
      s->x += i->wind;
    }
    if ((size_t)(15 + s->kind) >= i->background.count)
      continue;
    p = &i->background.sprites[15 + s->kind];
    if (s->x + p->w > 0 && s->x < WIDTH) {
      canvas_sprite(work, p, s->x, s->y, i->wind > 0 ? FLIP_H : 0, CLIP_FULL);
      redraw_over(i, work, (Rect){s->x, s->y, p->w, p->h});
      canvas_copy_rect_from(bg, work, s->x, s->y, p->w, p->h);
    }
  }
  free(work);
  return true;
}
bool island_init(Island *i, const JCArchive *a, Canvas *bg, Rng *r,
                 const IslandOptions *o, int32_t max_day, JCError *err) {
  JCPicture p = {0};
  char name[16];
  if (!o->background || i->mode == ISLAND_NONE)
    canvas_fill(bg, 5);
  else {
    if (i->mode == ISLAND_NORMAL && i->phase < 12)
      snprintf(name, sizeof name, "OCEAN0%u.SCR", rng_below(r, 3));
    else
      strcpy(name, "NIGHT.SCR");
    if (!jc_archive_scr(a, name, &p, err))
      return false;
    canvas_load_picture(bg, &p);
    jc_picture_free(&p);
  }
  if (i->tide == -1)
    i->tide = rng_next(r) & 1;
  place(i, r);
  if (i->mode != ISLAND_NONE) {
    i->raft_stage = max_day - 1;
    if (i->raft_stage < 1)
      i->raft_stage = 1;
    if (i->raft_stage > 5)
      i->raft_stage = 5;
    draw_raft(i, bg);
    sprite(i, bg, 0, island.x, island.y, 0, CLIP_FULL);
    if (i->tide == 1) {
      sprite(i, bg, 1, low_shore.x, low_shore.y, 0, CLIP_FULL);
      sprite(i, bg, 2, rock.x, rock.y, 0, CLIP_FULL);
    }
    i->waves[0] = i->waves[1] = i->waves[2] = 2;
    i->wave_timer = 0;
    update_waves(i);
    draw_waves(i, bg);
    sprite(i, bg, 14, shadow.x, shadow.y, 0, CLIP_FULL);
    sprite(i, bg, 13, trunk.x, trunk.y, 0, CLIP_FULL);
    sprite(i, bg, 12, leaves.x, leaves.y, 0, CLIP_FULL);
  }
  i->cap_count = o->waves && !i->freeze_waves ? 30 : 0;
  memset(i->caps, 0, sizeof i->caps);
  i->cap_quota[0] = 30;
  i->cap_quota[1] = 4;
  i->cap_quota[2] = 2;
  i->cap_quota[3] = 1;
  if (!init_clouds(i, bg, r, o->clouds ? 15 : 0)) {
    if (err)
      snprintf(err->message, sizeof err->message,
               "cloud staging allocation failed");
    return false;
  }
  return true;
}
static void whitecaps(Island *i, Canvas *c, Rng *r) {
  bool moving = i->cap_timer == 0;
  size_t n;
  uint16_t wp[] = {35, 5, 5, 5}, wi[] = {30, 50};
  for (n = 0; n < i->cap_count; ++n) {
    IslandSlot s = i->caps[n];
    int sprite_id = -1;
    if (empty(s) && rng_chance(r, 35)) {
      int p = (int)rng_pick(r, wp, 4);
      if (p == 1 && i->cap_quota[0] > 0) {
        int interval = rng_below(r, 30) + 10;
        int32_t x, y;
        Rect pt;
        rand_pos(r, NULL, (Rect){0, 0xd6, WIDTH, 0x23}, &x, &y);
        s = slot(x, y, 0, interval, interval);
        pt = (Rect){x, y, 1, 1};
        if (!overlap(pt, rect(i, trunk)) && !overlap(pt, rect(i, leaves)))
          --i->cap_quota[0];
        else
          memset(&s, 0, sizeof s);
      } else if (p >= 2 && p <= 4 && i->cap_quota[p - 1] > 0) {
        static const Rect areas[] = {{-10, 0xf9, 0x28a, 0x28},
                                     {-40, 0x121, 0x2a8, 0x5d},
                                     {-70, 0x17e, 0x2c6, 0x8e}};
        int kind = p - 1, interval = (int)rng_pick(r, wi, 2);
        int32_t x, y;
        rand_pos(r, NULL, areas[kind - 1], &x, &y);
        s = slot(x, y, kind, interval, interval);
        --i->cap_quota[kind];
      }
    }
    if (empty(s)) {
      i->caps[n] = s;
      continue;
    }
    if (!s.kind) {
      canvas_pixel(c, s.x, s.y, 15, CLIP_FULL);
      if (moving && slot_tick(&s)) {
        memset(&s, 0, sizeof s);
        ++i->cap_quota[0];
      }
    } else {
      if (moving) {
        s.x += i->wind;
        if (slot_tick(&s)) {
          s.counter = s.interval;
          ++s.frame;
        }
      }
      if (s.frame >= 0 && (size_t)s.frame < i->exe->whitecap_counts[3 - s.kind])
        sprite_id = i->exe->whitecap_seqs[3 - s.kind][s.frame];
      else {
        ++i->cap_quota[s.kind];
        memset(&s, 0, sizeof s);
      }
    }
    i->caps[n] = s;
    if (sprite_id >= 0 && (size_t)sprite_id < i->background.count) {
      const JCSprite *p = &i->background.sprites[sprite_id];
      Rect area = {s.x, s.y, p->w, p->h};
      const Wave *ws;
      size_t k, num;
      bool hidden = false;
      ws = wave_spots(i, &num);
      (void)num;
      for (k = 0; k < 3; ++k)
        if (overlap(area, rect(i, ws[k].r)) == 2)
          hidden = true;
      if (!hidden) {
        canvas_sprite(c, p, s.x, s.y, 0, CLIP_FULL);
        redraw_over(i, c, area);
      }
    }
  }
  if (moving)
    i->cap_timer = 10;
}
void island_step(Island *i, Canvas *c, Rng *r) {
  if (i->mode == ISLAND_NONE)
    return;
  whitecaps(i, c, r);
  if (!i->freeze_waves) {
    update_waves(i);
    draw_waves(i, c);
    draw_raft(i, c);
  }
}

void island_trace_layout(const Island *i, GfxTrace trace, void *user) {
  if (!trace || i->mode == ISLAND_NONE)
    return;
  const int ids[3] = {14, 13, 12};
  const Rect rects[3] = {shadow, trunk, leaves};
  for (unsigned n = 0; n < 3; ++n) {
    size_t id = (size_t)ids[n];
    if (id >= i->background.count)
      continue;
    GfxCommand c = {.op = GFX_SPRITE,
                    .domain = GFX_SCENERY,
                    .key = UINT64_C(0x8400000000000000) + id,
                    .sprite_id = (uint16_t)id,
                    .sprite = &i->background.sprites[id],
                    .x = i->x + rects[n].x,
                    .y = i->y + rects[n].y,
                    .clip = CLIP_FULL};
    trace(user, &c);
  }
}
