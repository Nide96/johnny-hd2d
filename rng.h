/* C99 adaptation of MIT open-scrantic (Glauber Lima), seg4:1e86. */
#ifndef JC_RNG_H
#define JC_RNG_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
  uint16_t table[56];
  size_t j, k;
} Rng;
void rng_init(Rng *, const uint16_t table[56], size_t j, size_t k);
void rng_warm_up(Rng *, uint64_t unix_time);
uint16_t rng_next(Rng *);
uint16_t rng_below(Rng *, uint16_t n);
size_t rng_pick(Rng *, const uint16_t *weights, size_t count);
bool rng_chance(Rng *, uint16_t percent);
#endif
