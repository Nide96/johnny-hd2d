/* seg3 ambient island, adapted from MIT open-scrantic. */
#ifndef JC_ISLAND_H
#define JC_ISLAND_H
#include "gfx.h"
#include "rng.h"
typedef enum { ISLAND_NORMAL, ISLAND_NIGHT, ISLAND_NONE } IslandMode;
typedef struct {
  int32_t x, y, frame, kind, interval, counter;
} IslandSlot;
typedef struct {
  bool background, clouds, waves;
} IslandOptions;
typedef struct {
  int32_t cfg, tide;
  bool no_raft, freeze_waves;
  IslandMode mode;
  bool holiday_ok;
  int32_t phase, holiday, x, y;
  uint32_t wave_timer, cap_timer;
  int32_t waves[3], wind, raft_stage;
  IslandSlot caps[30];
  size_t cap_count;
  int32_t cap_quota[4];
  JCBmp background, raft, holiday_bmp;
  const JCExe *exe;
} Island;
bool island_create(Island *, const JCArchive *, const JCExe *, JCError *);
void island_free(Island *);
void island_reset_config(Island *);
void island_apply_flags(Island *, uint32_t);
bool island_compatible(const Island *, uint32_t);
void island_ttm_offset(const Island *, bool shifted, int32_t *x, int32_t *y);
bool island_init(Island *, const JCArchive *, Canvas *, Rng *,
                 const IslandOptions *, int32_t max_day, JCError *);
/* Presentation-only static layer traces; never writes a canvas or consumes RNG.
 */
void island_trace_layout(const Island *, GfxTrace, void *);
void island_step(Island *, Canvas *, Rng *);
void island_draw_holiday(const Island *, Canvas *);
void island_trunk_overlay(const Island *, Canvas *, int32_t x, int32_t y,
                          int32_t w, int32_t h);
#endif
