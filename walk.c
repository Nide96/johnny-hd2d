/* MIT open-scrantic C99 adaptation. Original seg4:0074,0094,0b64. */
#include "walk.h"
#include <string.h>
void walk_init(Walk *w) {
  memset(w, 0, sizeof *w);
  w->spot = w->target = 1;
  w->target_hdg = -1;
}
void walk_reset(Walk *w) {
  w->has_route = w->has_path = false;
  w->path_start_hdg = w->hdg;
}
bool walk_go_to(Walk *w, int32_t s, int32_t h) {
  if (s)
    w->target = s;
  if (h != -1)
    w->target_hdg = h;
  return (!s || w->spot == s) && (h == -1 || w->hdg == h);
}
static void turn_frame(Walk *w, const JCExe *e, const uint8_t table[7]) {
  size_t id;
  w->has_frame = false;
  if (w->spot < 0 || w->spot >= 7 || w->hdg < 0)
    return;
  id = table[w->spot];
  w->has_frame = id < 35 && (size_t)w->hdg < e->paths[id].count;
  if (w->has_frame)
    w->frame = e->paths[id].frames[w->hdg];
}
static void advance(Walk *w, const JCExe *e, Rng *r) {
  if (!w->has_route && !w->has_path && w->spot != w->target) {
    w->has_route = true;
    w->route_from = (uint8_t)w->spot;
    w->route_to = (uint8_t)w->target;
  }
  if (w->has_route && !w->has_path && w->route_from < 7 && w->route_to < 7) {
    const JCRoute *q = &e->routes[w->route_from][w->route_to];
    size_t i, id;
    int32_t draw;
    for (i = 0; i < q->count && q->words[i] != -7 && q->words[i] != -w->spot;
         ++i) {
    }
    if (i >= q->count || q->words[i] == -7)
      goto destination_heading;
    ++i;
    draw = rng_below(r, 100);
    for (;;) {
      draw -= i + 1 < q->count ? q->words[i + 1] : 0;
      if (draw <= 0 || i + 2 >= q->count || q->words[i + 2] < 0)
        break;
      i += 2;
    }
    if (i >= q->count)
      goto destination_heading;
    id = q->words[i] > 0 ? (size_t)q->words[i] : 0;
    w->has_path = id > 0 && id < 35 && e->paths[id].count > 0;
    w->path_id = id;
    w->path_idx = 0;
    w->path_start_hdg = id < 35 ? e->path_info[id][0] : 0;
    w->path_end_hdg = id < 35 ? e->path_info[id][1] : 0;
    w->path_end_spot = id < 35 ? e->path_info[id][2] : 0;
    if (!w->has_path) {
      w->hdg = w->path_end_hdg;
      w->spot = w->path_end_spot;
      if (w->spot == w->target)
        w->has_route = false;
    }
  }
destination_heading:
  if (w->target_hdg != -1 && w->spot == w->target &&
      w->hdg == w->path_start_hdg) {
    if (w->path_start_hdg != w->target_hdg)
      w->path_start_hdg = w->target_hdg;
    else {
      turn_frame(w, e, e->stand_turns);
      w->target_hdg = -1;
    }
  }
  if (w->hdg != w->path_start_hdg) {
    int32_t d = w->hdg - w->path_start_hdg, dir;
    if (d == 4 || d == -4)
      dir = rng_next(r) & 1 ? -1 : 1;
    else
      dir = d < -4 || (d > 0 && d < 4) ? -1 : 1;
    w->hdg = (w->hdg + dir) & 7;
    turn_frame(w, e, w->spot == w->target ? e->stand_turns : e->walk_turns);
  }
  if (w->has_path && w->hdg == w->path_start_hdg) {
    const JCWalkPath *p = &e->paths[w->path_id];
    w->has_frame = w->path_idx < p->count;
    if (w->has_frame)
      w->frame = p->frames[w->path_idx];
    if (w->path_idx + 1 >= p->count) {
      w->hdg = w->path_end_hdg;
      w->spot = w->path_end_spot;
      w->has_path = false;
      if (w->spot == w->target)
        w->has_route = false;
    } else
      ++w->path_idx;
  }
}
void walk_step(Walk *w, const JCExe *e, Rng *r, const JCBmp *b,
               const Island *is, Canvas *c) {
  const JCSprite *s;
  int32_t x, y;
  if (!w->timer)
    advance(w, e, r);
  if (!w->has_frame || w->frame.sprite >= b->count)
    return;
  s = &b->sprites[w->frame.sprite];
  x = is->x + w->frame.x;
  y = is->y + w->frame.y;
  GfxDomain saved_domain = c->trace_domain;
  uint16_t saved_sprite = c->trace_sprite_id;
  const char *saved_resource = c->trace_resource;
  c->trace_domain = GFX_WALK;
  c->trace_sprite_id = w->frame.sprite;
  c->trace_resource = "JOHNWALK.BMP";
  canvas_sprite(c, s, x, y, w->frame.mirror ? FLIP_H : 0, CLIP_FULL);
  c->trace_domain = saved_domain;
  c->trace_sprite_id = saved_sprite;
  c->trace_resource = saved_resource;
  island_trunk_overlay(is, c, x, y, s->w, s->h);
  w->timer = 4;
}
