/* C99 adaptation of MIT open-scrantic (Glauber Lima), seg4:1e86, 1f1b. */
#include "rng.h"
#include <string.h>
void rng_init(Rng *r, const uint16_t table[56], size_t j, size_t k) {
  memcpy(r->table, table, sizeof r->table);
  r->j = j % 56;
  r->k = k % 56;
}
uint16_t rng_next(Rng *r) {
  uint16_t v = (uint16_t)(r->table[r->k] + r->table[r->j]);
  r->table[r->k] = v;
  r->k = (r->k + 1) % 56;
  r->j = (r->j + 1) % 56;
  return v;
}
void rng_warm_up(Rng *r, uint64_t t) {
  uint32_t i;
  for (i = 0; i < (t & 65535); ++i)
    (void)rng_next(r);
}
uint16_t rng_below(Rng *r, uint16_t n) {
  return n ? (uint16_t)(rng_next(r) % n) : 0;
}
size_t rng_pick(Rng *r, const uint16_t *weights, size_t n) {
  size_t i;
  uint16_t v = rng_next(r);
  for (i = 0; i < n; ++i) {
    uint16_t t = (uint16_t)(weights[i] * 655u);
    if (t > v)
      return i + 1;
    v = (uint16_t)(v - t);
  }
  return 0;
}
bool rng_chance(Rng *r, uint16_t p) { return rng_pick(r, &p, 1) == 1; }
