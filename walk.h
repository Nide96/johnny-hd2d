#ifndef JC_WALK_H
#define JC_WALK_H
#include "island.h"
typedef struct {
  int32_t spot, target, hdg, target_hdg;
  uint32_t timer;
  bool has_route, has_path, has_frame;
  uint8_t route_from, route_to;
  size_t path_id, path_idx;
  int32_t path_start_hdg, path_end_hdg, path_end_spot;
  JCWalkFrame frame;
} Walk;
void walk_init(Walk *);
void walk_reset(Walk *);
bool walk_go_to(Walk *, int32_t spot, int32_t hdg);
void walk_step(Walk *, const JCExe *, Rng *, const JCBmp *, const Island *,
               Canvas *);
#endif
