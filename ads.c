/* Clean C99 port of open-scrantic src/engine/ads.rs (MIT).
 * Copyright (c) 2026 Glauber Lima. See licenses/open-scrantic-MIT.txt.
 * Addresses refer to the original SCRANTIC.SCR Win16 executable.
 * Bounded instruction/recursion budgets reject malformed scripts. */
#include "ads.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  TAG_RUNNING = 1,
  TAG_STOPPED = 2,
  TAG_RESTART = 3,
  TAG_CONTINUE = 4,
  TAG_FIRST_RUN = 8
};
typedef struct AdsTag {
  uint16_t id, state, counter;
  size_t code, resume, *scenes, scene_count;
} AdsTag;
#define NONE SIZE_MAX

static void error(AdsRuntime *a, const char *msg) {
  a->failed = true;
  snprintf(a->error, sizeof a->error, "%s", msg);
}
static bool step_budget(AdsRuntime *a) {
  if (a->failed)
    return false;
  if (!a->budget) {
    error(a, "ADS: instruction budget exceeded");
    return false;
  }
  a->budget--;
  return true;
}
static uint16_t word(const AdsRuntime *a, size_t p) {
  if (p >= a->script.size || a->script.size - p < 2)
    return 0xFFFF;
  return (uint16_t)(a->script.data[p] | ((uint16_t)a->script.data[p + 1] << 8));
}
static int32_t arg(const AdsRuntime *a, size_t p, unsigned i) {
  uint16_t w = word(a, p + 2 + i * 2);
  return w < 0x8000u ? w : (int32_t)w - 65536;
}
static bool condition_op(uint16_t op) { return op >= 0x1310 && op <= 0x13C1; }
/* seg10:0303 – original instruction-length table. */
static size_t next(AdsRuntime *a, size_t p) {
  size_t len = 2;
  uint16_t op;
  if (p == NONE || p >= a->script.size || a->script.size - p < 2)
    return NONE;
  op = word(a, p);
  if (op == 0x2000 || op == 0x2005)
    len = 10;
  else if (op == 0x2010 || op == 0x2015 || op == 0x2020 || op == 0x4000 ||
           op == 0x4010)
    len = 8;
  else if ((op >= 0x1010 && op <= 0x1070) || (op >= 0x1310 && op <= 0x1370))
    len = 6;
  else if (op == 0xF010 || op == 0xF200 || op == 0xF210 || op == 0x1080 ||
           op == 0x1380 || op == 0x1390 || op == 0x13A0 || op == 0x13A1 ||
           op == 0x13B0 || op == 0x13B1 || op == 0x13C0 || op == 0x13C1 ||
           op == 0x3020)
    len = 4;
  if (len > a->script.size - p) {
    error(a, "ADS: truncated instruction");
    return NONE;
  }
  return p + len;
}
static size_t find_scene(const AdsRuntime *a, int32_t slot, int32_t tag) {
  size_t k;
  for (k = 0; k < a->scene_count; k++) {
    size_t i = a->order[k];
    if (a->scenes[i].slot == slot && a->scenes[i].tag == tag)
      return i;
  }
  return NONE;
}
static size_t tag_index(const AdsRuntime *a, int32_t id) {
  size_t i;
  for (i = 0; i < a->tag_count; i++)
    if (a->tags[i].id == id)
      return i;
  return NONE;
}
static bool collect_scenes(AdsRuntime *a, AdsTag *t) {
  size_t p = t->code;
  while (p != NONE && word(a, p) != 0xFFFF) {
    if (word(a, p) == 0x2000 || word(a, p) == 0x2005) {
      size_t s = find_scene(a, arg(a, p, 0), arg(a, p, 1)), i;
      if (s != NONE) {
        for (i = 0; i < t->scene_count; i++)
          if (t->scenes[i] == s)
            break;
        if (i == t->scene_count) {
          size_t *v = realloc(t->scenes, (i + 1) * sizeof *v);
          if (!v) {
            error(a, "ADS: scene list allocation failed");
            return false;
          }
          t->scenes = v;
          v[t->scene_count++] = s;
        }
      }
    }
    p = next(a, p);
  }
  return !a->failed;
}
void ads_free(AdsRuntime *a) {
  size_t i;
  for (i = 0; i < a->ttm_count; i++)
    ttm_ctx_free(&a->ttms[i]);
  for (i = 0; i < a->tag_count; i++)
    free(a->tags[i].scenes);
  jc_blob_free(&a->script);
  free(a->tags);
  free(a->ttms);
  free(a->scenes);
  free(a->order);
  memset(a, 0, sizeof *a);
}
bool ads_load(AdsRuntime *a, const JCArchive *archive, uint8_t id,
              const char *name) {
  JCAds src = {0};
  JCError err = {{0}};
  size_t i, j, p, start;
  memset(a, 0, sizeof *a);
  a->id = id;
  if (!jc_archive_ads(archive, name, &src, &err)) {
    error(a, err.message);
    jc_ads_free(&src);
    return false;
  }
  a->script = src.script;
  memset(&src.script, 0, sizeof src.script);
  if (a->script.size < 2) {
    error(a, "ADS: empty script");
    goto done;
  }
  a->ttms = calloc(src.ttm_count, sizeof *a->ttms);
  if (src.ttm_count && !a->ttms) {
    error(a, "ADS: TTM allocation failed");
    goto done;
  }
  a->ttm_count = src.ttm_count;
  for (i = 0; i < src.ttm_count; i++) {
    TtmCtx *c = &a->ttms[i];
    TtmScene *v;
    if (!jc_archive_ttm(archive, src.ttms[i].name, &c->ttm, &err)) {
      error(a, err.message);
      goto done;
    }
    v = realloc(a->scenes, (a->scene_count + c->ttm.scene_count) * sizeof *v);
    if (c->ttm.scene_count && !v) {
      error(a, "ADS: scene allocation failed");
      goto done;
    }
    if (v)
      a->scenes = v;
    for (j = 0; j < c->ttm.scene_count; j++) {
      JCSceneTag tag = c->ttm.scenes[j];
      ttm_scene_init(&a->scenes[a->scene_count++], i, src.ttms[i].id, tag.tag,
                     tag.frame);
    }
  }
  a->order = malloc(a->scene_count * sizeof *a->order);
  if (a->scene_count && !a->order) {
    error(a, "ADS: drawing-order allocation failed");
    goto done;
  }
  for (i = 0; i < a->scene_count; i++)
    a->order[i] = i;
  /* seg10:04fc: FFFF-separated sections, each begins with tag ID. */
  start = 2;
  p = 2;
  while (p != NONE && p < a->script.size) {
    if (word(a, p) == 0xFFFF) {
      AdsTag *v = realloc(a->tags, (a->tag_count + 1) * sizeof *v), *t;
      if (!v) {
        error(a, "ADS: tag allocation failed");
        goto done;
      }
      a->tags = v;
      t = &v[a->tag_count++];
      memset(t, 0, sizeof *t);
      t->id = word(a, start - 2);
      t->code = start;
      t->state = TAG_FIRST_RUN;
      t->resume = NONE;
      if (!collect_scenes(a, t))
        goto done;
      start = p + 4;
      p = start;
    } else
      p = next(a, p);
  }
  if (!a->tag_count)
    error(a, "ADS: no terminated tags");
done:
  jc_ads_free(&src);
  return !a->failed;
}
void ads_start_tag(AdsRuntime *a, uint8_t id) {
  size_t i = tag_index(a, id);
  if (i != NONE) {
    a->tags[i].resume = NONE;
    a->tags[i].state = (a->tags[i].state & TAG_FIRST_RUN) | TAG_RESTART;
  }
}
bool ads_tag_busy(const AdsRuntime *a, uint8_t id) {
  size_t i = tag_index(a, id), j;
  unsigned st;
  if (i == NONE || a->failed)
    return false;
  st = (unsigned)(a->tags[i].state & ~TAG_FIRST_RUN);
  if (st == TAG_CONTINUE || st == TAG_RUNNING)
    return true;
  for (j = 0; j < a->tags[i].scene_count; j++) {
    const TtmScene *s = &a->scenes[a->tags[i].scenes[j]];
    if (s->state != TTM_DONE && s->state != TTM_IDLE && !s->looping)
      return true;
  }
  return false;
}
/* seg10:1223 – condition values, shared by IF and WAIT forms. */
static bool condition(const AdsRuntime *a, uint16_t op, size_t p) {
  size_t i = find_scene(a, arg(a, p, 0), arg(a, p, 1));
  const TtmScene *s = i != NONE ? &a->scenes[i] : NULL;
  switch (op & 0xFF) {
  case 0x10:
    return s && s->state == TTM_FROZEN;
  case 0x20:
    return !s || s->state != TTM_FROZEN;
  case 0x30:
    return !s || s->plays == 0;
  case 0x40:
    return s && s->plays != 0;
  case 0x50:
    return s && s->state == TTM_DONE;
  case 0x60:
    return !s || s->state == TTM_DONE || s->state == TTM_IDLE;
  case 0x70:
    return s && s->state >= 1 && s->state <= 3;
  case 0x80:
    return a->tags[a->cur].counter <= arg(a, p, 0);
  default:
    return false; /* ADS variables are not used in SCRANTIC. */
  }
}
static size_t eval_cond(AdsRuntime *a, size_t p) {
  bool result = false, or_hit = false;
  for (;;) {
    if (!step_budget(a))
      return NONE;
    if (condition(a, word(a, p), p))
      result = true;
    for (;;) {
      uint16_t op;
      p = next(a, p);
      if (p == NONE)
        return NONE;
      op = word(a, p);
      if (op != 0x1430 && op != 0x1420) {
        a->cond = result || or_hit;
        return p;
      }
      if (result) {
        if (op == 0x1430) {
          or_hit = true;
          p = next(a, p);
          if (p == NONE)
            return NONE;
          continue;
        }
        result = false;
        p = next(a, p);
        break;
      }
      if (op == 0x1430) {
        p = next(a, p);
        break;
      }
      p = next(a, p);
      if (p == NONE)
        return NONE;
    }
    if (p == NONE)
      return NONE;
  }
}
static size_t skip_block(AdsRuntime *a, size_t p) {
  size_t out = NONE;
  if (++a->depth > 128) {
    error(a, "ADS: block nesting exceeds 128");
    a->depth--;
    return NONE;
  }
  while (p != NONE && step_budget(a)) {
    uint16_t op = word(a, p);
    if (op == 0 || op == 0xFFFF)
      break;
    if (op == 0x1510 || op == 0x1500) {
      out = p;
      break;
    }
    if (condition_op(op)) {
      size_t end = eval_cond(a, p), q;
      if (end == NONE)
        break;
      q = skip_block(a, end);
      if (q == NONE)
        break;
      if (word(a, q) == 0x1500)
        q = skip_block(a, q + 2);
      p = next(a, q);
    } else
      p = next(a, p);
  }
  a->depth--;
  return out;
}
static size_t skip_wait(AdsRuntime *a, size_t p) {
  while (p != NONE && step_budget(a)) {
    uint16_t op = word(a, p);
    if (op == 0 || op == 0xFFFF)
      return NONE;
    if (op == 0x1520)
      return p;
    p = next(a, p);
  }
  return NONE;
}
static size_t exec(AdsRuntime *, size_t, Rng *);
static size_t run_block(AdsRuntime *a, size_t p, Rng *rng) {
  size_t out = NONE;
  if (++a->depth > 128) {
    error(a, "ADS: block nesting exceeds 128");
    a->depth--;
    return NONE;
  }
  while (p != NONE && word(a, p) != 0xFFFF && step_budget(a)) {
    size_t r = exec(a, p, rng);
    if (a->brk) {
      a->brk = false;
      out = r;
      break;
    }
    p = r;
  }
  a->depth--;
  return out;
}
static int32_t weight(AdsRuntime *a, size_t p) {
  uint16_t op = word(a, p);
  return arg(a, p, op == 0x2000 || op == 0x2005 ? 3 : op == 0x3020 ? 0 : 2);
}
static size_t random_block(AdsRuntime *a, size_t start, Rng *rng) {
  int32_t total = 0, r;
  size_t p = start, after;
  while (p != NONE && word(a, p) != 0x30FF && step_budget(a)) {
    if (word(a, p) == 0xFFFF) {
      error(a, "ADS: unterminated RANDOM block");
      return NONE;
    }
    if ((int64_t)total + weight(a, p) > INT32_MAX ||
        (int64_t)total + weight(a, p) < INT32_MIN) {
      error(a, "ADS: RANDOM weight sum overflow");
      return NONE;
    }
    total += weight(a, p);
    p = next(a, p);
  }
  if (p == NONE || a->failed)
    return NONE;
  after = p + 2;
  if (total <= 0)
    return after;
  {
    uint16_t w = rng_below(rng, (uint16_t)total);
    r = w < 0x8000u ? w : 65536 - (int32_t)w;
  }
  r++;
  p = start;
  while (step_budget(a)) {
    r -= weight(a, p);
    if (r <= 0)
      break;
    p = next(a, p);
    if (p == NONE || word(a, p) == 0x30FF) {
      error(a, "ADS: invalid RANDOM weights");
      return NONE;
    }
  }
  if (!a->failed && word(a, p) != 0x3020)
    (void)exec(a, p, rng);
  return after;
}
static void add_scene(AdsRuntime *a, int32_t slot, int32_t tag, int32_t n) {
  size_t i = find_scene(a, slot, tag);
  TtmScene *s;
  if (i == NONE)
    return;
  s = &a->scenes[i];
  /* 2005 can reassert a running scene without resetting its frame. */
  if (s->state == TTM_IDLE || s->state == TTM_DONE)
    s->generation++;
  if (n == 0)
    s->state = TTM_ONCE;
  else if (n > 0) {
    s->state = TTM_REPEAT;
    s->repeats = n - 1;
  } else {
    s->state = TTM_TIMED;
    s->end_time = a->now + (uint32_t)(-n);
  }
  s->plays = (uint16_t)(s->plays + 1u);
}
/* seg10:1519 – one statement, NONE suspends/stops this tag. */
static size_t exec_inner(AdsRuntime *a, size_t p, Rng *rng);
static size_t exec(AdsRuntime *a, size_t p, Rng *rng) {
  size_t out;
  if (++a->depth > 128) {
    error(a, "ADS: statement nesting exceeds 128");
    a->depth--;
    return NONE;
  }
  out = exec_inner(a, p, rng);
  a->depth--;
  return out;
}
static size_t exec_inner(AdsRuntime *a, size_t p, Rng *rng) {
  uint16_t op = word(a, p);
  int32_t a0 = arg(a, p, 0), a1 = arg(a, p, 1), a2 = arg(a, p, 2);
  size_t i, q, r;
  switch (op) {
  case 0x0001:
  case 0x0005:
    return p + 2;
  case 0xFFFF:
    return NONE;
  case 0x1500:
    q = skip_block(a, p + 2);
    a->brk = true;
    return q;
  case 0x1510:
    a->brk = true;
    return p;
  case 0x1520:
    a->brk = true;
    return NONE;
  case 0x2000:
    i = find_scene(a, a0, a1);
    if (i != NONE) {
      a->scenes[i].cur = a->scenes[i].start;
      a->scenes[i].generation++;
    }
    /* fall through */
  case 0x2005:
    add_scene(a, a0, a1, a2);
    return p + 10;
  case 0x2010:
  case 0x2015:
  case 0x2020:
    i = find_scene(a, a0, a1);
    if (i != NONE) {
      if (op == 0x2010)
        a->scenes[i].state = TTM_IDLE;
      else if (op == 0x2015)
        a->scenes[i].state = TTM_FROZEN;
      else
        ttm_scene_reset(&a->scenes[i]);
    }
    return p + 8;
  case 0x4000:
  case 0x4010:
    i = find_scene(a, a0, a1);
    if (i != NONE) {
      for (q = 0; q < a->scene_count && a->order[q] != i; q++) {
      }
      memmove(&a->order[q], &a->order[q + 1],
              (a->scene_count - q - 1) * sizeof *a->order);
      if (op == 0x4000)
        a->order[a->scene_count - 1] = i;
      else {
        memmove(a->order + 1, a->order,
                (a->scene_count - 1) * sizeof *a->order);
        a->order[0] = i;
      }
    }
    return p + 8;
  case 0x3010:
    return random_block(a, p + 2, rng);
  case 0xF000:
    a->tags[a->cur].state = TAG_STOPPED;
    return NONE;
  case 0xF010:
    i = a0 == -1 ? a->cur : tag_index(a, a0);
    if (i != NONE) {
      a->tags[i].state = TAG_STOPPED;
      if (i == a->cur)
        return NONE;
    }
    return p + 4;
  case 0xF200:
  case 0xF210:
    i = tag_index(a, a0);
    if (i != NONE) {
      AdsTag *t = &a->tags[i];
      t->resume = NONE;
      t->state = (t->state & TAG_FIRST_RUN) |
                 (op == 0xF200 ? TAG_CONTINUE : TAG_RESTART);
    }
    return p + 4;
  default:
    if (condition_op(op)) {
      q = eval_cond(a, p);
      if (q == NONE)
        return NONE;
      if (a->cond)
        r = run_block(a, q, rng);
      else {
        r = skip_block(a, q);
        if (r != NONE && word(a, r) == 0x1500)
          r = run_block(a, r + 2, rng);
      }
      return r == NONE ? NONE : r + 2;
    }
    if (op >= 0x1010 && op <= 0x1080 && (op & 15) == 0) {
      q = eval_cond(a, p);
      if (q == NONE)
        return NONE;
      if (a->cond) {
        AdsTag *t = &a->tags[a->cur];
        t->counter = (uint16_t)(t->counter + 1u);
        t->resume = p;
        (void)run_block(a, q, rng);
        return NONE;
      }
      a->tags[a->cur].counter = 0;
      a->tags[a->cur].resume = NONE;
      r = skip_wait(a, q);
      return r == NONE ? NONE : r + 2;
    }
    a->failed = true;
    snprintf(a->error, sizeof a->error,
             "ADS tag %u byte %zu: unsupported opcode %04x", a->tags[a->cur].id,
             p, op);
    return NONE;
  }
}
/* seg10:0c30 – re-entry follows the first 0005 marker, if present. */
static size_t reentry(AdsRuntime *a, size_t start) {
  size_t p = start;
  while (p != NONE && word(a, p) != 0xFFFF) {
    if (word(a, p) == 0x0005)
      return p + 2;
    p = next(a, p);
  }
  return start;
}
/* seg10:1acb – propagate, re-evaluate ADS, draw all running TTM frames. */
bool ads_update(AdsRuntime *a, VmEnv *e) {
  size_t t, k;
  bool ran = false;
  if (a->failed)
    return false;
  a->now = e->now;
  a->budget = 1000000;
  a->depth = 0;
  for (t = 0; t < a->tag_count; t++) {
    uint16_t flags = (uint16_t)(a->tags[t].state & ~TAG_FIRST_RUN);
    for (k = 0; k < a->tags[t].scene_count; k++) {
      TtmScene *s = &a->scenes[a->tags[t].scenes[k]];
      if (flags == TAG_RESTART)
        ttm_scene_reset(s);
      else
        s->tag_state = flags;
    }
  }
  for (t = 0; t < a->tag_count && !a->failed; t++) {
    AdsTag *tag = &a->tags[t];
    uint16_t st = tag->state;
    size_t p = tag->code;
    if (st & TAG_FIRST_RUN) {
      st = (uint16_t)(st & ~TAG_FIRST_RUN);
      tag->state = st;
    } else
      p = reentry(a, p);
    if (tag->resume != NONE)
      p = tag->resume;
    if (st == TAG_RESTART || st == TAG_CONTINUE) {
      tag->state = TAG_RUNNING;
      st = TAG_RUNNING;
    }
    a->cur = t;
    if (st == TAG_RUNNING)
      (void)run_block(a, p, e->rng);
  }
  for (k = 0; k < a->scene_count && !a->failed; k++) {
    TtmScene *s = &a->scenes[a->order[k]];
    uint16_t tag_state = s->tag_state, old_state = s->state;
    bool running = old_state == TTM_ONCE || old_state == TTM_REPEAT ||
                   old_state == TTM_TIMED || old_state == TTM_FROZEN;
    if (tag_state != 6 && running) {
      TtmFrameResult r;
      if (!ttm_run_frame(&a->ttms[s->ttm], s, e, &r)) {
        if (e->failed) {
          error(a, e->error);
          break;
        }
        if (tag_state != 5) {
          s->jump = s->start;
          s->state = TTM_DONE;
        }
      } else if (tag_state != 5) {
        s->redraw = true;
        ran = true;
        if (r.has_delay && r.delay != s->delay) {
          s->next_time = a->now + (uint32_t)r.delay;
          s->delay = r.delay;
        }
        if (r.purge) {
          s->jump = s->start;
          if (s->state == TTM_REPEAT && s->repeats != 0) {
            if (ttm_scene_advance(s, a->now))
              s->repeats--;
          } else if (s->state == TTM_TIMED && s->end_time != 0)
            (void)ttm_scene_advance(s, a->now);
          else if (ttm_scene_advance(s, a->now)) {
            s->state = TTM_DONE;
            s->delay = 0;
          }
        } else {
          if (r.has_jump && r.jump != -1) {
            s->jump = r.jump;
            if (s->cur == r.jump)
              s->looping = true;
          }
          if (s->state != TTM_FROZEN)
            (void)ttm_scene_advance(s, a->now);
        }
      }
    } else if (tag_state != 6 && tag_state != 5 && old_state == TTM_DONE)
      s->state = TTM_IDLE;
    if (old_state == TTM_TIMED && s->end_time <= a->now)
      s->state = TTM_DONE;
  }
  return ran;
}
