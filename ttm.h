#ifndef JC_TTM_H
#define JC_TTM_H
/* C99 port of open-scrantic (MIT), Copyright (c) 2026 Glauber Lima.
 * See licenses/open-scrantic-MIT.txt for the permission notice. */
#include "gfx.h"
#include "resource.h"
#include "rng.h"

typedef struct {
  Canvas *work, *bg, *front;
  const JCArchive *archive;
  const JCBmp *johnwalk;
  Rng *rng;
  void (*sound)(void *, uint32_t);
  void *sound_user;
  int32_t offset_x, offset_y;
  uint32_t now; /* units of 16 ms, never wall-clock milliseconds */
  bool failed;
  char error[256];
} VmEnv;

enum { TTM_IDLE, TTM_ONCE, TTM_REPEAT, TTM_TIMED, TTM_DONE, TTM_FROZEN };
typedef struct {
  size_t ttm;
  uint16_t slot, tag;
  uint32_t generation; /* activation identity: renderer snaps across restarts */
  int32_t start, jump, cur;
  bool looping, redraw;
  unsigned bmp_slot;
  uint8_t fg, bg;
  bool clip_on;
  Clip clip;
  int32_t delay;
  uint16_t tag_state, plays, state;
  int32_t repeats;
  uint32_t end_time, next_time;
} TtmScene;

typedef struct {
  JCTtm ttm;
  JCBmp owned_bmps[6];
  const JCBmp *bmps[6];
  char bmp_names[6][14]; /* normalized resource identity for presentation */
  uint8_t *saved;
  size_t saved_len;
  Clip saved_zone;
} TtmCtx;
typedef struct {
  bool has_delay, purge, has_jump;
  int32_t delay, jump;
} TtmFrameResult;

void ttm_ctx_free(TtmCtx *);
void ttm_scene_init(TtmScene *, size_t ttm, uint16_t slot, uint16_t tag,
                    size_t frame);
void ttm_scene_reset(TtmScene *);
bool ttm_scene_advance(TtmScene *, uint32_t now);
/* False means frame outside script, or env->failed with detailed error. */
bool ttm_run_frame(TtmCtx *, TtmScene *, VmEnv *, TtmFrameResult *);
#endif
