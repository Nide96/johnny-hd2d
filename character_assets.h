#ifndef JC_CHARACTER_ASSETS_H
#define JC_CHARACTER_ASSETS_H
#include "render_frame.h"
#include "generated/reading_bounds.h"
#include "generated/fishing_bounds.h"
#include <string.h>
/* Explicit resource/frame lookup. Partial bodies and props are never guessed.
 */
enum {
  JC_HD_NONE = -1,
  JC_HD_WALK,
  JC_HD_POSES,
  JC_HD_REST,
  JC_HD_SLEEP,
  JC_HD_READ,
  JC_HD_READ_PROPS,
  JC_HD_READ_WALK,
  JC_HD_FISH,
  JC_HD_FISH_PARTS,
  JC_HD_FISH_REACTIONS,
  JC_HD_FISH_REACTION_PARTS,
  JC_HD_SHEET_COUNT,
  JC_HD_ACTION_COUNT = JC_HD_FISH - JC_HD_POSES
};
typedef struct {
  int sheet, cell;
  bool mirror, fit_rect, shadow;
} JCHDPose;
typedef struct {
  float x, y, w, h;
} JCHDRect;
enum { JC_SYMBOL_NONE, JC_SYMBOL_QUESTION = 2, JC_SYMBOL_EXCLAMATION = 3 };
static inline int jc_symbol_kind(const JCDraw *d) {
  if (!strcmp(d->resource, "LITEBULB.BMP")) {
    if (d->source_sprite == 4)
      return JC_SYMBOL_QUESTION;
    if (d->source_sprite == 1)
      return JC_SYMBOL_EXCLAMATION;
  }
  return JC_SYMBOL_NONE;
}
/* Storage padding is not part of an accessory's visible size. Reflection is
 * around the complete VM sprite rectangle, before projecting its alpha bounds.
 */
static inline JCHDRect jc_reading_footprint(unsigned id, uint8_t flags,
                                            JCHDRect rect) {
  if (id < 36) {
    const JCReadingBounds *b = &reading_bounds[id];
    float sx = rect.w / b->width, sy = rect.h / b->height;
    rect.x += ((flags & FLIP_H) ? b->width - b->x - b->w : b->x) * sx;
    rect.y += ((flags & FLIP_V) ? b->height - b->y - b->h : b->y) * sy;
    rect.w = b->w * sx;
    rect.h = b->h * sy;
  }
  return rect;
}
static inline JCHDPose jc_character_pose(const JCDraw *d) {
  JCHDPose p = {JC_HD_NONE, 0, false, false, true};
  if (!strcmp(d->resource, "JOHNWALK.BMP")) {
    unsigned id = d->source_sprite;
    if (id >= 1 && id <= 8) {
      p.sheet = JC_HD_WALK;
      p.cell = (int)((id + 4u) % 8u);
    } else if (id >= 24 && id <= 29) {
      p.sheet = JC_HD_READ_WALK;
      p.cell = (int)id - 24;
      p.mirror =
          false; /* v2 is authored in the original left/front direction */
    } else {
      static const unsigned ids[10] = {0, 9, 10, 11, 12, 14, 15, 16, 17, 18};
      for (unsigned i = 0; i < 10; ++i)
        if (id == ids[i]) {
          p.sheet = JC_HD_POSES;
          p.cell = (int)i;
          /* Generated rear quarter pose faces right; original 11 faces left. */
          p.mirror = id == 11;
          break;
        }
    }
  } else if (!strcmp(d->resource, "STNDLAY.BMP") && d->source_sprite < 8) {
    p.sheet = JC_HD_REST;
    p.cell = d->source_sprite;
    p.fit_rect = true;
  } else if (!strcmp(d->resource, "SLEEP.BMP") && d->source_sprite < 17 &&
             d->source_sprite != 12) {
    p.sheet = JC_HD_SLEEP;
    p.cell = d->source_sprite - (d->source_sprite > 12);
    p.fit_rect = true;
  } else if (!strcmp(d->resource, "MJREAD.BMP") && d->source_sprite < 36) {
    unsigned id = d->source_sprite;
    p.sheet = id < 24 ? JC_HD_READ : JC_HD_READ_PROPS;
    p.cell = (int)(id < 24 ? id : id - 24);
    p.fit_rect = true;
    p.shadow = id <= 18 || (id >= 21 && id <= 23) || (id >= 26 && id <= 30);
  } else if (!strcmp(d->resource, "MJFISH1.BMP") && d->source_sprite < 30) {
    static const unsigned body[] = {0, 1, 2,  3,  4,  5,  6,  7,
                                    8, 9, 10, 11, 12, 13, 16, 17};
    static const unsigned parts[] = {14, 15, 18, 19, 20, 21, 22,
                                     23, 24, 25, 26, 27, 28, 29};
    unsigned id = d->source_sprite;
    /* These three generated fragments still differ from the original gesture. */
    if (id == 19 || id == 28 || id == 29) return p;
    for (unsigned i = 0; i < sizeof body / sizeof *body; ++i)
      if (id == body[i]) {
        p.sheet = JC_HD_FISH;
        p.cell = (int)i;
      }
    for (unsigned i = 0; i < sizeof parts / sizeof *parts; ++i)
      if (id == parts[i]) {
        p.sheet = JC_HD_FISH_PARTS;
        p.cell = (int)i;
      }
    p.fit_rect = true;
    p.shadow =
        (id >= 3 && id <= 13) || id == 16 || id == 17 || id == 20 || id == 21;
  } else if (!strcmp(d->resource, "MJFISH2.BMP") && d->source_sprite < 27) {
    static const unsigned body[] = {0, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 24};
    static const unsigned parts[] = {1,  2,  4,  5,  15, 16, 17, 18,
                                     19, 20, 21, 22, 23, 25, 26};
    unsigned id = d->source_sprite;
    for (unsigned i = 0; i < sizeof body / sizeof *body; ++i)
      if (id == body[i]) {
        p.sheet = JC_HD_FISH_REACTIONS;
        p.cell = (int)i;
      }
    for (unsigned i = 0; i < sizeof parts / sizeof *parts; ++i)
      if (id == parts[i]) {
        p.sheet = JC_HD_FISH_REACTION_PARTS;
        p.cell = (int)i;
      }
    p.fit_rect = true;
    p.shadow = id == 3 || (id >= 6 && id <= 14) || id == 24;
  }
  return p;
}
/* Fit visible pixels rather than BMP storage padding. In particular a hand,
 * a fishing line and a whole body must keep their own original footprint. */
static inline JCHDRect jc_fishing_footprint(const JCDraw *d, uint8_t flags,
                                            JCHDRect r) {
  const JCReadingBounds *b = NULL;
  if (!strcmp(d->resource, "MJFISH1.BMP") && d->source_sprite < 30)
    b = &fishing1_bounds[d->source_sprite];
  else if (!strcmp(d->resource, "MJFISH2.BMP") && d->source_sprite < 27)
    b = &fishing2_bounds[d->source_sprite];
  if (b) {
    float sx = r.w / b->width, sy = r.h / b->height;
    r.x += ((flags & FLIP_H) ? b->width - b->x - b->w : b->x) * sx;
    r.y += ((flags & FLIP_V) ? b->height - b->y - b->h : b->y) * sy;
    r.w = b->w * sx;
    r.h = b->h * sy;
  }
  return r;
}
#endif
