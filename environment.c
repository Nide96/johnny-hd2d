#include "environment.h"
#include "layout.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static float clamp01(float x) { return fminf(1.f, fmaxf(0.f, x)); }
static float smooth(float a, float b, float x) {
  x = clamp01((x - a) / (b - a));
  return x * x * (3.f - 2.f * x);
}
JCAtmosphere jc_atmosphere(double hour) {
  hour = fmod(hour, 24.0);
  if (hour < 0)
    hour += 24;
  float elevation = sinf((float)((hour - 6) * 3.141592653589793 / 12));
  float day = smooth(-.16f, .12f, elevation);
  float dusk = (1.f - smooth(.05f, .55f, fabsf(elevation))) * day;
  float night = 1.f - smooth(-.20f, .02f, elevation);
  JCAtmosphere a = {{0}, {0}, {0}, {0}, {day, night, dusk, 0}};
  const float nt[3] = {.008f, .015f, .045f}, dt[3] = {.12f, .32f, .57f},
              st[3] = {.18f, .16f, .33f};
  const float nh[3] = {.045f, .075f, .13f}, dh[3] = {.68f, .79f, .84f},
              sh[3] = {1.f, .43f, .18f};
  for (unsigned i = 0; i < 3; ++i) {
    a.top[i] = nt[i] + (dt[i] + (st[i] - dt[i]) * dusk - nt[i]) * day;
    a.horizon[i] = nh[i] + (dh[i] + (sh[i] - dh[i]) * dusk - nh[i]) * day;
  }
  a.top[3] = a.horizon[3] = 1;
  a.sun[0] = .12f + .76f * clamp01((float)(hour - 6) / 12);
  a.sun[1] = .315f - fmaxf(elevation, 0) * .245f;
  a.sun[2] = day;
  a.sun[3] = dusk;
  float mp = (float)fmod(hour + 6, 24) / 12;
  a.moon[0] = .12f + .76f * clamp01(mp);
  a.moon[1] = .315f - fmaxf(-elevation, 0) * .245f;
  a.moon[2] = night;
  a.moon[3] = .72f;
  return a;
}
bool jc_environment_wind(unsigned scene, bool walk_preview) {
  if (walk_preview)
    return true;
  /* Conservative full-scene exclusions from the original scene inventory:
   * trunk interaction, coconut/leaf gag and scripted crown replacements. */
  switch (scene) {
  case 0:
  case 1:
  case 3:
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
  case 11:
  case 12:
  case 13:
  case 16:
  case 17:
  case 18:
  case 19:
  case 20:
  case 21:
  case 22:
  case 23:
  case 24:
  case 25:
  case 26:
  case 27:
  case 30:
  case 31:
  case 32:
  case 33:
  case 35:
  case 51:
  case 52:
  case 53:
  case 76:
  case 78:
    return false;
  default:
    return scene < 80;
  }
}
static float source_y(float y) {
  if (y < 240)
    return y * 224 / 240;
  if (y < 425)
    return 224 + (y - 240) * 117 / 185;
  if (y < 592)
    return 341 + (y - 425) * 147 / 167;
  return 488 + (y - 592) * 232 / 128;
}
static void flood(uint8_t *dst, const uint8_t *allowed, int *queue, int start) {
  const int n = JC_ENV_W * JC_ENV_H;
  if (start < 0 || start >= n || !allowed[start] || dst[start])
    return;
  int head = 0, tail = 0;
  dst[start] = 1;
  queue[tail++] = start;
  while (head < tail) {
    int p = queue[head++], x = p % JC_ENV_W, y = p / JC_ENV_W;
    int next[4] = {x ? p - 1 : -1, x + 1 < JC_ENV_W ? p + 1 : -1,
                   y ? p - JC_ENV_W : -1, y + 1 < JC_ENV_H ? p + JC_ENV_W : -1};
    for (unsigned k = 0; k < 4; ++k)
      if (next[k] >= 0 && allowed[next[k]] && !dst[next[k]]) {
        dst[next[k]] = 1;
        queue[tail++] = next[k];
      }
  }
}
uint8_t *jc_environment_mask(const uint8_t *rgba, int width, int height) {
  if (!rgba || width < 1 || height < 1)
    return NULL;
  const size_t n = JC_ENV_W * JC_ENV_H;
  uint8_t *a = calloc(n, 1), *b = calloc(n, 1), *land = calloc(n, 1),
          *out = malloc(n * 4);
  int *queue = malloc(n * sizeof *queue);
  float *distance = malloc(n * sizeof *distance);
  if (!a || !b || !land || !out || !queue || !distance) {
    free(a);
    free(b);
    free(land);
    free(out);
    free(queue);
    free(distance);
    return NULL;
  }
  for (int y = 0; y < JC_ENV_H; ++y)
    for (int x = 0; x < JC_ENV_W; ++x) {
      float wx = (x + .5f) * 2, wy = (y + .5f) * 2;
      float dx = (wx - 650) / 430, dy = (wy - 505) / 111;
      int sx = (int)(wx * width / 1280),
          sy = (int)(source_y(wy) * height / 720);
      if (sy < 0)
        sy = 0;
      if (sy >= height)
        sy = height - 1;
      const uint8_t *c = rgba + ((size_t)sy * width + sx) * 4;
      a[y * JC_ENV_W + x] = (uint8_t)(dx * dx + dy * dy < 1 &&
                                      c[0] > c[1] + 9 && c[1] > c[2] + 9);
    }
  /* Opening cuts narrow golden reflection bridges before island flood fill. */
  for (int y = 1; y < JC_ENV_H - 1; ++y)
    for (int x = 1; x < JC_ENV_W - 1; ++x) {
      int p = y * JC_ENV_W + x;
      b[p] = a[p] && a[p - 1] && a[p + 1] && a[p - JC_ENV_W] && a[p + JC_ENV_W];
    }
  flood(land, b, queue, 252 * JC_ENV_W + 320);
  memset(a, 0, n);
  for (int y = 1; y < JC_ENV_H - 1; ++y)
    for (int x = 1; x < JC_ENV_W - 1; ++x) {
      int p = y * JC_ENV_W + x;
      a[p] = land[p] || land[p - 1] || land[p + 1] || land[p - JC_ENV_W] ||
             land[p + JC_ENV_W];
    }
  for (size_t i = 0; i < n; ++i)
    b[i] = (uint8_t)!a[i];
  memset(land, 0, n);
  flood(land, b, queue, 0);
  for (size_t i = 0; i < n; ++i)
    a[i] = (uint8_t)!land[i]; /* Fill enclosed sand holes. */
  for (int y = 0; y < JC_ENV_H; ++y)
    for (int x = 0; x < JC_ENV_W; ++x) {
      int p = y * JC_ENV_W + x;
      bool edge = false;
      if (x && a[p] != a[p - 1])
        edge = true;
      if (x + 1 < JC_ENV_W && a[p] != a[p + 1])
        edge = true;
      if (y && a[p] != a[p - JC_ENV_W])
        edge = true;
      if (y + 1 < JC_ENV_H && a[p] != a[p + JC_ENV_W])
        edge = true;
      distance[p] = edge ? 0.f : 10000.f;
    }
  for (int pass = 0; pass < 2; ++pass)
    for (int k = 0; k < (int)n; ++k) {
      int p = pass ? (int)n - 1 - k : k, x = p % JC_ENV_W, y = p / JC_ENV_W;
      for (int yy = -1; yy <= 1; ++yy)
        for (int xx = -1; xx <= 1; ++xx) {
          if (!xx && !yy)
            continue;
          int qx = x + xx, qy = y + yy;
          if (qx < 0 || qy < 0 || qx >= JC_ENV_W || qy >= JC_ENV_H)
            continue;
          int q = qy * JC_ENV_W + qx;
          if ((!pass && q >= p) || (pass && q <= p))
            continue;
          float candidate = distance[q] + (xx && yy ? 1.41421356f : 1.f);
          if (candidate < distance[p])
            distance[p] = candidate;
        }
    }
  for (int y = 0; y < JC_ENV_H; ++y)
    for (int x = 0; x < JC_ENV_W; ++x) {
      int p = y * JC_ENV_W + x;
      float d = fminf(distance[p] * 2, 32) * (a[p] ? 1 : -1);
      float rx = ((x + .5f) * 2 - 155) / 64, ry = ((y + .5f) * 2 - 548) / 35;
      out[p * 4] = (uint8_t)lroundf((.5f + d / 64) * 255);
      out[p * 4 + 1] = a[p] ? 255 : 0;
      out[p * 4 + 2] = rx * rx + ry * ry < 1 ? 255 : 0;
      out[p * 4 + 3] = 255;
    }
  free(a);
  free(b);
  free(land);
  free(queue);
  free(distance);
  return out;
}
void jc_environment_fire(const JCRenderFrame *f, float out[4]) {
  memset(out, 0, 4 * sizeof *out);
  for (size_t i = 0; i < f->actor_count; ++i) {
    const JCDraw *d = &f->actors[i];
    if (strcmp(d->resource, "FIRE.BMP") && strcmp(d->resource, "FIRE1.BMP") &&
        strcmp(d->resource, "FIRE2.BMP") && strcmp(d->resource, "FIRE3.BMP") &&
        strcmp(d->resource, "FIRE4.BMP"))
      continue;
    unsigned yellow = 0, red = 0;
    double xx = 0, yy = 0;
    for (int y = 0; y < d->h; ++y)
      for (int x = 0; x < d->w; ++x) {
        int dx = (d->flags & FLIP_H) ? d->w - 1 - x : x,
            dy = (d->flags & FLIP_V) ? d->h - 1 - y : y;
        if (d->x + dx < d->clip.x1 || d->x + dx > d->clip.x2 ||
            d->y + dy < d->clip.y1 || d->y + dy > d->clip.y2)
          continue;
        uint8_t c = f->atlas[(size_t)(d->ay + y) * JC_ATLAS_W + d->ax + x];
        if (c == 14) {
          ++yellow;
          xx += dx + .5;
          yy += dy + .5;
        }
        if (c == 12 || c == 4)
          ++red;
      }
    if (yellow >= 5 && red >= 5) {
      out[0] = jc_layout_x(d->x + (float)(xx / yellow) - f->origin_x);
      out[1] = jc_layout_y(d->y + (float)(yy / yellow) - f->origin_y);
      out[2] = 1;
      out[3] = 115;
      return;
    }
  }
}
