#ifndef JC_HYBRID_H
#define JC_HYBRID_H
#include "layout.h"
#include "character_assets.h"
#include "sprite_material.h"
#include <math.h>
#ifndef JC_STREAM_MIB
#define JC_STREAM_MIB 2
#endif
#define JC_COPY_MIB 1
#define JC_RELAY_MIB 2
typedef struct {
  float tint[4], sun[4], skew;
} JCHybridLight;
static inline JCHybridLight jc_hybrid_light(double hour) {
  float angle = (float)((hour - 6.0) * 3.141592653589793 / 12.0);
  float height = sinf(angle),
        day = fminf(1.f, fmaxf(0.f, (height + .15f) / .4f));
  day = day * day * (3.f - 2.f * day);
  float x = -cosf(angle), ambient = .5f + .5f * day;
  JCHybridLight l = {{ambient * (.78f + .22f * day),
                      ambient * (.88f + .10f * day), ambient, .12f},
                     {x, -.7f, day, .14f + .18f * day},
                     -x * (.4f + .8f * (1.f - fmaxf(height, 0.f)))};
  return l;
}
/* No generated character textures in any story or preview path. */
static inline bool jc_hybrid_hd_sequence(const JCRenderFrame *f) {
  (void)f;
  return false;
}
/* Character resources, including the detached body parts used by TTM.
 * Scenery, punctuation, fish, ships and the other human characters keep their
 * existing material. Mixed original sprites keep their authored assembly. */
static inline bool jc_hybrid_johnny(const JCDraw *d) {
  if (!strcmp(d->resource, "JOHNWALK.BMP") ||
      !strcmp(d->resource, "MJ_AMB.BMP"))
    return true;
  if (!strcmp(d->resource, "STNDLAY.BMP"))
    return d->source_sprite < 9;
  if (!strcmp(d->resource, "SLEEP.BMP"))
    return d->source_sprite < 17;
  if (!strcmp(d->resource, "MJREAD.BMP"))
    return d->source_sprite <= 30 && d->source_sprite != 20;
  if (!strcmp(d->resource, "MJFISH1.BMP"))
    return d->source_sprite < 22 || d->source_sprite >= 27;
  if (!strcmp(d->resource, "MJFISH2.BMP"))
    return true;
  static const char *const families[] = {
      "COCOHEAD.BMP", "DRUNKJON.BMP", "FIRE.BMP",     "FIRE3.BMP",
      "FIRE4.BMP",    "GJANGRY.BMP",  "GJCASTLE.BMP", "GJCATCH1.BMP",
      "GJCATCH2.BMP", "GJCATCH3.BMP", "GJDIVE.BMP",   "GJFFFOOD.BMP",
      "GJGULL1.BMP",  "GJGULL1A.BMP", "GJGULL2.BMP",  "GJGULL2A.BMP",
      "GJGULL3.BMP",  "GJGULL3A.BMP", "GJHOT.BMP",    "GJKINGKO.BMP",
      "GJNAT1.BMP",   "GJNAT1LI.BMP", "GJNAT3.BMP",   "GJPROW.BMP",
      "GJRUNAWA.BMP", "GJVIS3.BMP",   "GJVIS5.BMP",   "GJVIS52.BMP",
      "GJVIS6.BMP",   "JATA.BMP",     "JCHANGE.BMP",  "JOHNWOUL.BMP",
      "MEXCWALK.BMP", "MJBATH.BMP",   "MJBOTTLE.BMP", "MJFISH3.BMP",
      "MJBTL2.BMP",   "MJCOCO.BMP",   "MJDIVE.BMP",   "MJJOG1.BMP",
      "MJJOG2.BMP",   "MJRAFT2.BMP",  "MJSANDC.BMP",  "MJTELE.BMP",
      "MJTELE2.BMP",  "SHARKWLK.BMP", "SHKNFIST.BMP", "SJBRAKUP.BMP",
      "SJGFTASK.BMP", "SJGFTJMP.BMP", "SJGFTSHY.BMP", "SJGFTXCH.BMP",
      "SJMSUZY1.BMP", "SJMSUZY2.BMP", "SJMSUZY3.BMP", "SJRAFT1.BMP",
      "SJWORK.BMP",   "SLEVEJC1.BMP", "SLEVEJC2.BMP", "SLEVEJC3.BMP",
      "WOULDBE.BMP"};
  for (size_t i = 0; i < sizeof families / sizeof *families; ++i)
    if (!strcmp(d->resource, families[i]))
      return true;
  return false;
}
static inline JCHDPose jc_hybrid_pose(const JCDraw *d) {
  JCHDPose p = jc_character_pose(d);
  if (p.sheet >= JC_HD_FISH)
    p.sheet = JC_HD_NONE;
  return p;
}
static inline bool jc_hybrid_ground(float x, float y) {
  float dx = (x - 645.f) / 343.f, dy = (y - 505.f) / 85.f;
  return dx * dx + dy * dy < .98f * .98f;
}
static inline bool jc_hybrid_shadow(const JCDraw *d, float x, float y, float w,
                                    float h) {
  if ((!strcmp(d->resource, "SLEEP.BMP") && d->source_sprite == 12) ||
      (!strcmp(d->resource, "STNDLAY.BMP") && d->source_sprite == 8) ||
      (!strcmp(d->resource, "JOHNWALK.BMP") && d->source_sprite == 13))
    return false; /* Detached head/hand never owns the ground shadow. */
  JCHDPose known = jc_character_pose(d);
  if (known.sheet != JC_HD_NONE && !known.shadow &&
      (strcmp(d->resource, "MJREAD.BMP") || d->source_sprite != 19))
    return false;
  if (!strcmp(d->resource, "MJFISH1.BMP") &&
      (d->source_sprite == 19 || d->source_sprite == 28 ||
       d->source_sprite == 29))
    return false;
  return d->transparent && (d->resource[0] || d->domain == GFX_RAFT) &&
         !(d->flags & FLIP_V) && d->domain != GFX_AMBIENT &&
         d->domain != GFX_OCCLUDER && d->w > 5 && d->h > 8 &&
         jc_hybrid_ground(x + w * .5f, y + h);
}
static inline float jc_hybrid_bottom(const JCRenderFrame *f, const JCDraw *d) {
  JCVisibleBounds b = jc_sprite_body_bounds(f, d);
  return b.bottom > 0 ? (float)b.bottom / d->h : 1.f;
}
static inline bool jc_hybrid_contact(const JCDraw *d, JCVisibleBounds b) {
  if (d->domain == GFX_RAFT)
    return true;
  if (!strcmp(d->resource, "SLEEP.BMP"))
    return true;
  if (!strcmp(d->resource, "STNDLAY.BMP"))
    return d->source_sprite >= 1;
  if (!strcmp(d->resource, "MJREAD.BMP"))
    return d->source_sprite < 29;
  /* A long fishing rod must not classify a standing body as horizontal. */
  if (!strncmp(d->resource, "MJFISH", 6) || d->walk)
    return false;
  return b.bottom - b.top <= (b.right - b.left) * .75f;
}
static inline void jc_hybrid_color(const uint8_t in[3], uint8_t out[4]) {
  float r = in[0] / 255.f, g = in[1] / 255.f, b = in[2] / 255.f;
  float lum = .299f * r + .587f * g + .114f * b;
  float c[3] = {.68f * r + .32f * lum, .68f * g + .32f * lum,
                .68f * b + .32f * lum};
  if (r < .02f && g < .02f && b < .02f) {
    c[0] = .14f;
    c[1] = .085f;
    c[2] = .055f;
  } else if (r > .65f && g < .35f && b < .35f) {
    c[0] = .80f;
    c[1] = .39f;
    c[2] = .24f;
  } else if (g > r * 1.3f && g > b * 1.2f) {
    c[0] = .18f + g * .12f;
    c[1] = .25f + g * .37f;
    c[2] = .18f + g * .10f;
  } else if (r > .6f && g > .45f && b < .35f) {
    c[0] = .87f;
    c[1] = .69f;
    c[2] = .37f;
  } else if (b > r * 1.3f && b > g * 1.2f) {
    c[0] = .16f;
    c[1] = .29f + b * .16f;
    c[2] = .36f + b * .25f;
  }
  for (int k = 0; k < 3; ++k)
    out[k] = (uint8_t)lroundf(fminf(1.f, .035f + c[k] * .93f) * 255.f);
  out[3] = 255;
}
#endif
