/* Clean C99 port of open-scrantic src/engine/ttm.rs (MIT).
 * Copyright (c) 2026 Glauber Lima. See licenses/open-scrantic-MIT.txt.
 * Original scene record and interpreter: seg10:0b3e, seg10:196e, seg12:040c.
 * Retains the original first-run/redraw distinction; no window or audio I/O. */
#include "ttm.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int32_t signed_word(uint16_t x) {
  return x < 0x8000u ? x : (int32_t)x - 65536;
}
static bool same_name(const char *a, const char *b) {
  while (*a && *b) {
    if (toupper((unsigned char)*a++) != toupper((unsigned char)*b++))
      return false;
  }
  return *a == *b;
}
static void vm_error(VmEnv *e, const char *msg) {
  e->failed = true;
  snprintf(e->error, sizeof e->error, "%s", msg);
}
void ttm_ctx_free(TtmCtx *c) {
  unsigned i;
  for (i = 0; i < 6; i++)
    jc_bmp_free(&c->owned_bmps[i]);
  jc_ttm_free(&c->ttm);
  free(c->saved);
  memset(c, 0, sizeof *c);
}
void ttm_scene_reset(TtmScene *s) {
  s->generation++;
  s->bmp_slot = 0;
  s->cur = s->start;
  s->jump = -1;
  s->fg = 15;
  s->bg = 15;
  s->delay = 0;
  s->next_time = 0;
  s->repeats = 0;
  s->plays = 0;
  s->redraw = false;
  s->state = TTM_IDLE;
  s->tag_state = 0;
  s->looping = false;
  s->clip_on = true;
  s->clip = CLIP_FULL;
}
void ttm_scene_init(TtmScene *s, size_t ttm, uint16_t slot, uint16_t tag,
                    size_t frame) {
  memset(s, 0, sizeof *s);
  s->ttm = ttm;
  s->slot = slot;
  s->tag = tag;
  s->start = (int32_t)frame;
  ttm_scene_reset(s);
}
bool ttm_scene_advance(TtmScene *s, uint32_t now) {
  /* seg10:196e. Unsigned next_time mirrors upstream including timer-wrap
   * comparison. */
  if (s->delay) {
    if (s->next_time > now)
      return false;
    s->next_time = now + (uint32_t)s->delay;
  }
  s->redraw = false;
  if (s->jump != -1) {
    s->cur = s->jump;
    s->jump = -1;
  } else
    s->cur++;
  return true;
}
static Canvas *buffer(VmEnv *e, int32_t i) {
  return i == 0 ? e->front : i == 2 ? e->bg : e->work;
}

bool ttm_run_frame(TtmCtx *ctx, TtmScene *s, VmEnv *e, TtmFrameResult *r) {
  const JCFrame *frame;
  size_t i;
  uint32_t sprite_ordinal = 0;
  bool skip = s->redraw;
  memset(r, 0, sizeof *r);
  if (s->cur < 0 || (size_t)s->cur >= ctx->ttm.frame_count)
    return false;
  frame = &ctx->ttm.frames[s->cur];
  for (i = 0; i < frame->count && !e->failed; i++) {
    const JCOp *op = &frame->ops[i];
    int32_t a[6];
    unsigned j;
    Clip clip;
    for (j = 0; j < 6; j++)
      a[j] = op->args[j];
    if ((op->code >= 0xA000 && op->code <= 0xAFFF) || op->code == 0x4004 ||
        op->code == 0x4204) {
      switch (op->code) {
      case 0x4004:
      case 0xA0A4:
        a[2] += e->offset_x;
        a[3] += e->offset_y;
        /* fall through */
      default:
        a[0] += e->offset_x;
        a[1] += e->offset_y;
        break;
      case 0xA601:
      case 0xAF1F:
      case 0xAF2F:
        break;
      }
    }
    clip = s->clip_on ? s->clip : CLIP_FULL;
    switch (op->code) {
    case 0x0FF0:
      return true;
    case 0xC051:
      if (!skip && e->sound)
        e->sound(e->sound_user, (uint32_t)a[0]);
      break;
    case 0xA002:
      canvas_pixel(e->work, a[0], a[1], s->fg, clip);
      break;
    case 0xA0A4:
      canvas_line(e->work, a[0], a[1], a[2], a[3], s->fg, clip);
      break;
    case 0xA104:
    case 0xA114:
      canvas_rect(e->work, a[0], a[1], a[2], a[3], s->fg, s->bg,
                  op->code == 0xA104, clip);
      break;
    case 0xA404:
    case 0xA424: {
      int32_t radius = a[3] >> 1;
      canvas_ellipse(e->work, a[0] + radius, a[1] + radius, radius, s->fg,
                     s->bg, clip);
      break;
    }
    case 0xA504:
    case 0xA514:
    case 0xA524:
    case 0xA534:
      e->work->trace_key = ((uint64_t)s->slot << 48) |
                           ((uint64_t)s->tag << 32) | sprite_ordinal++;
      e->work->trace_key ^=
          (uint64_t)s->generation * UINT64_C(0x9e3779b97f4a7c15);
      if (a[3] >= 0 && a[3] < 6 && ctx->bmps[a[3]] && a[2] >= 0 &&
          (size_t)a[2] < ctx->bmps[a[3]]->count) {
        const char *saved_name = e->work->trace_resource;
        uint16_t saved_id = e->work->trace_sprite_id;
        e->work->trace_resource = ctx->bmp_names[a[3]];
        e->work->trace_sprite_id = (uint16_t)a[2];
        canvas_sprite(e->work, &ctx->bmps[a[3]]->sprites[a[2]], a[0], a[1],
                      (uint8_t)((op->code & 0xF0) >> 4), clip);
        e->work->trace_resource = saved_name;
        e->work->trace_sprite_id = saved_id;
      }
      break;
    case 0xA054:
    case 0xA064:
      if (skip || a[2] <= 0 || a[3] <= 0 || a[0] + a[2] > 640 ||
          a[1] + a[3] > 480)
        break;
      if (op->code == 0xA054) {
        uint8_t *save;
        size_t count = 0;
        Clip zone = clip_rect(a[0], a[1], a[2], a[3]);
        save = canvas_save_rect(e->bg, zone, &count);
        if (!save && count) {
          vm_error(e, "TTM: cannot allocate saved zone");
          break;
        }
        free(ctx->saved);
        ctx->saved = save;
        ctx->saved_len = count;
        ctx->saved_zone = zone;
      } else if (ctx->saved)
        canvas_restore_rect(e->work, ctx->saved_zone, ctx->saved,
                            ctx->saved_len);
      break;
    case 0x0110:
      r->purge = true;
      break;
    case 0x1021:
      r->has_delay = true;
      r->delay = a[0];
      break;
    case 0x2022:
      if (!skip) {
        int32_t rem = signed_word(rng_below(e->rng, (uint16_t)(a[1] - a[0])));
        r->has_delay = true;
        r->delay = a[0] + (rem < 0 ? -rem : rem);
      }
      break;
    case 0x1051:
      s->bmp_slot = (unsigned)(a[0] < 0 ? 0 : a[0] > 5 ? 5 : a[0]);
      break;
    case 0x1201: {
      size_t f = jc_ttm_frame_of_tag(&ctx->ttm, a[0]);
      r->has_jump = true;
      r->jump = f == SIZE_MAX ? -1 : (int32_t)f;
      break;
    }
    case 0x2002:
      s->fg = (uint8_t)a[0];
      s->bg = (uint8_t)a[1];
      break;
    case 0x4004:
      s->clip = (Clip){a[0], a[1], a[2], a[3]};
      s->clip_on = true;
      break;
    case 0x4204:
      if (!skip)
        canvas_copy_rect_from(e->bg, e->work, a[0], a[1], a[2], a[3]);
      break;
    case 0xB606:
      if (!skip) {
        Canvas *src = buffer(e, a[4]), *dst = buffer(e, a[5]);
        if (src != dst)
          canvas_copy_rect_from(dst, src, a[0], a[1], a[2], a[3]);
      }
      break;
    case 0x0080:
      if (!skip) {
        jc_bmp_free(&ctx->owned_bmps[s->bmp_slot]);
        ctx->bmps[s->bmp_slot] = NULL;
        ctx->bmp_names[s->bmp_slot][0] = '\0';
      }
      break;
    case 0xF01F:
      if (!skip && op->text) {
        JCPicture pic = {0};
        JCError err = {{0}};
        if (!jc_archive_scr(e->archive, op->text, &pic, &err))
          vm_error(e, err.message);
        else {
          canvas_load_picture(e->work, &pic);
          memcpy(e->bg->px, e->work->px, sizeof e->bg->px);
        }
        jc_picture_free(&pic);
      }
      break;
    case 0xF02F:
      if (!skip && op->text) {
        unsigned slot = s->bmp_slot;
        JCError err = {{0}};
        jc_bmp_free(&ctx->owned_bmps[slot]);
        ctx->bmps[slot] = NULL;
        ctx->bmp_names[slot][0] = '\0';
        if (same_name(op->text, "JOHNWALK.BMP"))
          ctx->bmps[slot] = e->johnwalk;
        else if (!jc_archive_bmp(e->archive, op->text, &ctx->owned_bmps[slot],
                                 &err))
          vm_error(e, err.message);
        else
          ctx->bmps[slot] = &ctx->owned_bmps[slot];
        if (ctx->bmps[slot]) {
          size_t n;
          for (n = 0; op->text[n] && n < sizeof ctx->bmp_names[slot] - 1; n++)
            ctx->bmp_names[slot][n] = (char)toupper((unsigned char)op->text[n]);
          ctx->bmp_names[slot][n] = '\0';
        }
      }
      break;
    /* Tags, palettes, region declarations, frame setup; full recomposition
     * replaces their dirty-rectangle bookkeeping (documented upstream). */
    case 0x1061:
    case 0x1101:
    case 0x1111:
    case 0x1121:
    case 0x2012:
    case 0x4214:
    case 0xA601:
    case 0xAF1F:
    case 0xAF2F:
    case 0xF03F:
    case 0xF05F:
      break;
    default:
      e->failed = true;
      snprintf(e->error, sizeof e->error,
               "TTM frame %d op %zu: unsupported opcode %04x", s->cur, i,
               op->code);
      break;
    }
  }
  return !e->failed;
}
