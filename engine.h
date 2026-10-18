/* Story scheduler, C99 adaptation of MIT open-scrantic. No window/audio I/O. */
#ifndef JC_ENGINE_H
#define JC_ENGINE_H
#include "gfx.h"
#include "resource.h"
#define JC_TICK_MS 16u
#define JC_STEP_MS 55u
typedef struct {
  int32_t year, month, day, hour, minute;
} JCClock;
typedef struct {
  bool background, clouds, waves, sounds;
  int32_t num_days, intro, intro_saved, year, month, day, start_time;
} JCConfig;
typedef struct JCEngine JCEngine;
typedef struct {
  int32_t origin_x, origin_y;
  bool modern_allowed, walk_preview;
  unsigned scene_id; /* Read-only presentation metadata, original catalog ID. */
} JCWorldView;
JCWorldView jc_engine_world_view(const JCEngine *);
JCConfig jc_config_default(void);
/* Assets are borrowed and must outlive the engine. Seed is the original warmup
 * time. */
JCEngine *jc_engine_create(const JCAssets *, const JCConfig *, uint64_t seed,
                           JCError *);
void jc_engine_destroy(JCEngine *);
/* Isolated art preview: alternate two original side-walk paths; no story. */
void jc_engine_walk_preview(JCEngine *);
void jc_engine_tick(JCEngine *);
bool jc_engine_step(JCEngine *, const JCClock *, JCError *);
const Canvas *jc_engine_frame(const JCEngine *);
const uint8_t (*jc_engine_palette(const JCEngine *))[3];
const JCConfig *jc_engine_config(const JCEngine *);
uint64_t jc_engine_scenes_done(const JCEngine *);
/* Changes whenever a session or ADS runtime is replaced (presentation cuts). */
uint64_t jc_engine_epoch(const JCEngine *);
uint32_t jc_engine_now(const JCEngine *);
size_t jc_engine_scene_count(const JCEngine *);
bool jc_engine_force_scene(JCEngine *, size_t);
/* IDs requested during last step; newest sound interrupts previous (Win
 * sndPlaySound). Samples remain available in borrowed assets.exe.sounds. */
const uint32_t *jc_engine_sounds(const JCEngine *, size_t *count);
/* Trace excludes initial background memcpy. Callback sees composition in order;
 * engine_base is that background snapshot. Callback must not retain transient
 * sprite pointers. */
void jc_engine_set_trace(JCEngine *, GfxTrace, void *);
const Canvas *jc_engine_base(const JCEngine *);
#endif
