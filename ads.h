#ifndef JC_ADS_H
#define JC_ADS_H
/* C99 port of open-scrantic (MIT), Copyright (c) 2026 Glauber Lima. */
#include "ttm.h"
struct AdsTag;
typedef struct {
  uint8_t id;
  JCBlob script;
  struct AdsTag *tags;
  size_t tag_count;
  TtmCtx *ttms;
  size_t ttm_count;
  TtmScene *scenes;
  size_t scene_count;
  size_t *order;
  size_t cur;
  bool brk, cond, failed;
  uint32_t now;
  size_t budget;
  unsigned depth;
  char error[256];
} AdsRuntime;
/* Zero-initialize before first load. Free old runtime before loading another.
 */
bool ads_load(AdsRuntime *, const JCArchive *, uint8_t id, const char *name);
void ads_free(AdsRuntime *);
void ads_start_tag(AdsRuntime *, uint8_t id);
bool ads_tag_busy(const AdsRuntime *, uint8_t id);
/* True if a scene drew. Inspect failed/error independently. */
bool ads_update(AdsRuntime *, VmEnv *);
#endif
