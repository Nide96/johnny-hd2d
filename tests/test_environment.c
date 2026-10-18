#include "environment.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void atmosphere(void) {
  JCAtmosphere noon = jc_atmosphere(12), night = jc_atmosphere(0),
               wrap = jc_atmosphere(24);
  assert(noon.factors[0] > .99f && noon.factors[1] < .01f);
  assert(night.factors[0] < .01f && night.factors[1] > .99f);
  assert(!memcmp(&night, &wrap, sizeof night));
  assert(noon.sun[1] < .1f && night.moon[1] < .1f);
  for (int i = 1; i <= 1440; ++i) {
    JCAtmosphere a = jc_atmosphere((i - 1) / 60.0), b = jc_atmosphere(i / 60.0);
    for (unsigned j = 0; j < 3; ++j) {
      assert(isfinite(b.top[j]) && isfinite(b.horizon[j]));
      assert(fabsf(a.top[j] - b.top[j]) < .02f);
      assert(fabsf(a.horizon[j] - b.horizon[j]) < .04f);
    }
    assert(b.factors[0] >= 0 && b.factors[0] <= 1);
    assert(b.factors[1] >= 0 && b.factors[1] <= 1);
  }
  assert(!jc_environment_wind(35, false) && !jc_environment_wind(52, false));
  assert(jc_environment_wind(34, false) && jc_environment_wind(35, true));
  assert(!jc_environment_wind(0, false));
}
static void mask(void) {
  uint8_t *plate = malloc(1280 * 720 * 4);
  assert(plate);
  for (int y = 0; y < 720; ++y)
    for (int x = 0; x < 1280; ++x) {
      uint8_t *c = plate + ((size_t)y * 1280 + x) * 4;
      bool land = x >= 400 && x < 900 && y >= 360 && y < 470;
      c[0] = land ? 215 : 20;
      c[1] = land ? 160 : 80;
      c[2] = land ? 95 : 140;
      c[3] = 255;
      if (x >= 600 && x < 620 && y >= 410 && y < 430)
        c[0] = c[1] = c[2] = 30;
      if (x == 640 && y >= 470 && y < 540) {
        c[0] = 220;
        c[1] = 160;
        c[2] = 70;
      }
    }
  uint8_t *m = jc_environment_mask(plate, 1280, 720);
  assert(m);
  size_t center = ((size_t)252 * JC_ENV_W + 320) * 4;
  size_t ocean = ((size_t)320 * JC_ENV_W + 320) * 4;
  assert(m[center] > 128 && m[center + 1] == 255);
  assert(m[ocean] < 128 && m[ocean + 1] == 0);
  assert(m[((size_t)259 * JC_ENV_W + 305) * 4 + 1] ==
         255); /* Enclosed hole filled. */
  assert(m[((size_t)274 * JC_ENV_W + 77) * 4 + 2] == 255); /* Rock exclusion. */
  assert(!jc_environment_mask(NULL, 1, 1));
  free(m);
  free(plate);
}
static void fire(void) {
  JCRenderFrame *f = calloc(1, sizeof *f);
  assert(f);
  f->atlas = calloc(2048, 16);
  assert(f->atlas);
  f->actor_count = 1;
  JCDraw *d = f->actors;
  d->w = d->h = 4;
  d->x = 440;
  d->y = 280;
  d->clip = ((Clip){0, 0, 639, 479});
  strcpy(d->resource, "FIRE.BMP");
  for (int y = 0; y < 4; ++y)
    for (int x = 0; x < 4; ++x)
      f->atlas[y * 2048 + x] = (x < 2) ? 14 : 12;
  float glow[4];
  jc_environment_fire(f, glow);
  assert(glow[2] == 1 && glow[0] > 630 && glow[0] < 650);
  strcpy(d->resource, "FIRE1.BMP");
  jc_environment_fire(f, glow);
  assert(glow[2] == 1);
  d->clip.y2 = 279;
  jc_environment_fire(f, glow);
  assert(glow[2] == 0);
  d->clip = ((Clip){0, 0, 639, 479});
  strcpy(d->resource, "MJREAD.BMP");
  jc_environment_fire(f, glow);
  assert(glow[2] == 0);
  free(f->atlas);
  free(f);
}
int main(void) {
  atmosphere();
  mask();
  fire();
  puts("environment: continuous day/night, star visibility factors, full-scene "
       "wind locks, shore topology/distance, rock exclusion, real-flame gating "
       "OK");
  return 0;
}
