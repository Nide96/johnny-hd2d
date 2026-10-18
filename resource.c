/* Asset decoder ported from open-scrantic to C99.
 * MIT License
 *
 * Copyright (c) 2026 Glauber Lima
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#include "resource.h"
#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool fail(JCError *e, const char *fmt, ...) {
  va_list a;
  if (e) {
    va_start(a, fmt);
    vsnprintf(e->message, sizeof e->message, fmt, a);
    va_end(a);
  }
  return false;
}
static bool span(size_t n, size_t p, size_t z) { return p <= n && z <= n - p; }
static uint16_t u16(const uint8_t *d) {
  return (uint16_t)(d[0] | ((uint16_t)d[1] << 8));
}
static uint32_t u32(const uint8_t *d) {
  return (uint32_t)d[0] | ((uint32_t)d[1] << 8) | ((uint32_t)d[2] << 16) |
         ((uint32_t)d[3] << 24);
}
static bool word(const uint8_t *d, size_t n, size_t p, uint16_t *v,
                 JCError *e) {
  if (!span(n, p, 2))
    return fail(e, "truncated word at %zu", p);
  *v = u16(d + p);
  return true;
}
static char *text_copy(const uint8_t *d, size_t n) {
  char *s = malloc(n + 1);
  if (s) {
    memcpy(s, d, n);
    s[n] = 0;
  }
  return s;
}
static size_t zero_length(const uint8_t *d, size_t n) {
  const uint8_t *p = memchr(d, 0, n);
  return p ? (size_t)(p - d) : n;
}
static bool same_name(const char *a, const char *b) {
  while (*a && *b) {
    if (toupper((unsigned char)*a++) != toupper((unsigned char)*b++))
      return false;
  }
  return *a == *b;
}
void jc_blob_free(JCBlob *b) {
  if (b) {
    free(b->data);
    memset(b, 0, sizeof *b);
  }
}
void jc_archive_free(JCArchive *a) {
  if (a) {
    free(a->files);
    jc_blob_free(&a->volume);
    memset(a, 0, sizeof *a);
  }
}
const JCResource *jc_archive_find(const JCArchive *a, const char *n) {
  size_t i;
  for (i = 0; i < a->count; i++)
    if (same_name(a->files[i].name, n))
      return &a->files[i];
  return NULL;
}
bool jc_archive_parse(JCArchive *a, const uint8_t *m, size_t mn,
                      const uint8_t *v, size_t vn, JCError *e) {
  size_t i, count;
  memset(a, 0, sizeof *a);
  if (!span(mn, 19, 2))
    return fail(e, "RESOURCE.MAP too small");
  count = u16(m + 19);
  if (!span(mn, 21, count * 8))
    return fail(e, "truncated RESOURCE.MAP entries");
  a->files = calloc(count ? count : 1, sizeof *a->files);
  a->volume.data = malloc(vn ? vn : 1);
  if (!a->files || !a->volume.data) {
    jc_archive_free(a);
    return fail(e, "out of memory");
  }
  memcpy(a->volume.data, v, vn);
  a->volume.size = vn;
  a->count = count;
  for (i = 0; i < count; i++) {
    size_t off = u32(m + 21 + i * 8 + 4), z, j, len;
    JCResource *r = &a->files[i];
    if (!span(vn, off, 17)) {
      jc_archive_free(a);
      return fail(e, "MAP entry %zu out of range", i);
    }
    z = u32(v + off + 13);
    if (!span(vn, off + 17, z)) {
      jc_archive_free(a);
      return fail(e, "resource %zu data out of range", i);
    }
    len = zero_length(v + off, 13);
    for (j = 0; j < len; j++)
      r->name[j] = (char)toupper(v[off + j]);
    r->data = a->volume.data + off + 17;
    r->size = z;
  }
  return true;
}
typedef struct {
  const uint8_t *d;
  size_t n, pos;
  uint32_t buf;
  unsigned bits;
} Bits;
static bool bits_get(Bits *b, unsigned n, uint32_t *v) {
  while (b->bits < n) {
    if (b->pos >= b->n)
      return false;
    b->buf |= (uint32_t)b->d[b->pos++] << b->bits;
    b->bits += 8;
  }
  *v = b->buf & ((1u << n) - 1);
  b->buf >>= n;
  b->bits -= n;
  return true;
}
bool jc_decompress(const uint8_t *d, size_t n, JCBlob *out, JCError *e) {
  size_t size, w = 0, p = 5;
  uint8_t *o;
  memset(out, 0, sizeof *out);
  if (n < 5)
    return fail(e, "compressed chunk too small");
  size = u32(d + 1);
  if (size > JC_ASSET_MAX_UNPACKED)
    return fail(e, "decompressed size too large");
  if (d[0] != 1 && d[0] != 2)
    return fail(e, "unknown compression method %u", d[0]);
  o = malloc(size ? size : 1);
  if (!o)
    return fail(e, "out of memory");
  if (d[0] == 1) {
    while (w < size && p < n) {
      unsigned c = d[p++], z = c & 127;
      if (c & 128) {
        if (p >= n)
          goto bad;
        memset(o + w, d[p++], z > size - w ? size - w : z);
        w += z > size - w ? size - w : z;
      } else {
        if (!span(n, p, z))
          goto bad;
        if (z > size - w)
          z = (unsigned)(size - w);
        memcpy(o + w, d + p, z);
        p += z;
        w += z;
      }
    }
  } else if (size) {
    uint16_t prefix[4096] = {0};
    uint8_t suffix[4096] = {0}, stack[4096];
    Bits b = {d + 5, n - 5, 0, 0, 0};
    unsigned width = 9;
    size_t next = 257, old, code;
    uint32_t val, bitpos = 0;
    uint8_t last;
    if (!bits_get(&b, width, &val) || val > 255)
      goto bad;
    old = val;
    last = (uint8_t)val;
    o[w++] = last;
    while (w < size) {
      size_t depth = 0;
      unsigned guard = 0;
      if (!bits_get(&b, width, &val) || val > next)
        goto bad;
      code = val;
      bitpos += width;
      if (code == 256) {
        uint32_t spanbits = width * 8,
                 skip = (spanbits - ((bitpos - 1) % spanbits)) - 1;
        while (skip) {
          unsigned z = skip < 16 ? skip : 16;
          if (!bits_get(&b, z, &val))
            goto bad;
          skip -= z;
        }
        width = 9;
        next = 256;
        bitpos = 0;
        continue;
      }
      if (code >= next) {
        stack[depth++] = last;
        code = old;
      }
      while (code > 255) {
        if (code >= 4096 || depth >= 4095 || ++guard >= 4096)
          goto bad;
        stack[depth++] = suffix[code];
        code = prefix[code];
      }
      stack[depth++] = (uint8_t)code;
      last = (uint8_t)code;
      while (depth && w < size)
        o[w++] = stack[--depth];
      if (next < 4096) {
        prefix[next] = (uint16_t)old;
        suffix[next] = last;
        next++;
        if (next >= (1u << width) && width < 12) {
          width++;
          bitpos = 0;
        }
      }
      old = val;
    }
  }
  if (w != size)
    goto bad;
  out->data = o;
  out->size = size;
  return true;
bad:
  free(o);
  return fail(e, "invalid or truncated compressed payload");
}
typedef struct {
  const uint8_t *d;
  size_t n;
} View;
static bool chunk(const JCArchive *a, const char *name, const char *tag,
                  View *out, JCError *e) {
  const JCResource *r = jc_archive_find(a, name);
  size_t p = 0;
  bool found = false;
  if (!r)
    return fail(e, "missing resource %s", name);
  while (span(r->size, p, 8)) {
    const uint8_t *t = r->data + p;
    uint32_t z = u32(t + 4);
    p += 8;
    if (z & 0x80000000u)
      continue;
    if (!span(r->size, p, z))
      return fail(e, "%s: invalid chunk", name);
    if (!memcmp(t, tag, 4)) {
      out->d = r->data + p;
      out->n = z;
      found = true;
    }
    p += z;
  }
  if (p != r->size)
    return fail(e, "%s: truncated chunk header", name);
  return found ? true : fail(e, "%s: missing %.4s chunk", name, tag);
}
static bool unpack(const JCBlob *b, size_t first, size_t count, uint8_t **out,
                   JCError *e) {
  size_t i;
  if (first > JC_ASSET_MAX_UNPACKED * 2u ||
      count > JC_ASSET_MAX_UNPACKED * 2u - first)
    return fail(e, "image too large");
  *out = malloc(count ? count : 1);
  if (!*out)
    return fail(e, "out of memory");
  for (i = 0; i < count; i++) {
    size_t q = first + i;
    uint8_t v = q / 2 < b->size ? b->data[q / 2] : 0;
    (*out)[i] = (q & 1) ? v & 15 : v >> 4;
  }
  return true;
}
void jc_bmp_free(JCBmp *b) {
  size_t i;
  if (b) {
    for (i = 0; b->sprites && i < b->count; i++)
      free(b->sprites[i].px);
    free(b->sprites);
    memset(b, 0, sizeof *b);
  }
}
void jc_picture_free(JCPicture *p) {
  if (p) {
    free(p->px);
    memset(p, 0, sizeof *p);
  }
}
bool jc_archive_bmp(const JCArchive *a, const char *n, JCBmp *out, JCError *e) {
  View inf, bin;
  JCBlob b = {0};
  size_t i, first = 0;
  memset(out, 0, sizeof *out);
  if (!chunk(a, n, "INF:", &inf, e) || !chunk(a, n, "BIN:", &bin, e) ||
      inf.n < 2)
    return fail(e, "%s: bad BMP header", n);
  out->count = u16(inf.d);
  if (!span(inf.n, 2, out->count * 4))
    goto bad;
  if (!jc_decompress(bin.d, bin.n, &b, e))
    goto bad;
  out->sprites = calloc(out->count ? out->count : 1, sizeof *out->sprites);
  if (!out->sprites)
    goto bad;
  for (i = 0; i < out->count; i++) {
    JCSprite *s = &out->sprites[i];
    size_t z;
    s->w = u16(inf.d + 2 + i * 2);
    s->h = u16(inf.d + 2 + out->count * 2 + i * 2);
    z = (size_t)s->w * (size_t)s->h;
    if (!unpack(&b, first, z, &s->px, e))
      goto bad;
    first += z;
  }
  jc_blob_free(&b);
  return true;
bad:
  jc_blob_free(&b);
  jc_bmp_free(out);
  return fail(e, "%s: invalid BMP", n);
}
bool jc_archive_scr(const JCArchive *a, const char *n, JCPicture *out,
                    JCError *e) {
  View dim, bin;
  JCBlob b = {0};
  memset(out, 0, sizeof *out);
  if (!chunk(a, n, "DIM:", &dim, e) || dim.n < 4 ||
      !chunk(a, n, "BIN:", &bin, e))
    return fail(e, "%s: invalid SCR", n);
  out->w = u16(dim.d);
  out->h = u16(dim.d + 2);
  if (!jc_decompress(bin.d, bin.n, &b, e))
    return false;
  if (!unpack(&b, 0, (size_t)out->w * (size_t)out->h, &out->px, e)) {
    jc_blob_free(&b);
    return false;
  }
  jc_blob_free(&b);
  return true;
}
bool jc_archive_palette(const JCArchive *a, const char *n, uint8_t pal[256][3],
                        JCError *e) {
  View v;
  size_t i;
  if (!chunk(a, n, "VGA:", &v, e))
    return false;
  for (i = 0; i < 768; i++) {
    uint8_t x = i < v.n ? v.d[i] & 63 : 0;
    pal[i / 3][i % 3] = (uint8_t)((x << 2) | (x >> 4));
  }
  return true;
}
void jc_ttm_free(JCTtm *t) {
  size_t i, j;
  if (t) {
    for (i = 0; i < t->frame_count; i++) {
      for (j = 0; j < t->frames[i].count; j++)
        free(t->frames[i].ops[j].text);
      free(t->frames[i].ops);
    }
    free(t->frames);
    free(t->scenes);
    memset(t, 0, sizeof *t);
  }
}
static bool append(void **p, size_t count, size_t z, JCError *e) {
  void *q;
  if (count > SIZE_MAX / z)
    return fail(e, "allocation overflow");
  q = realloc(*p, count * z);
  if (!q)
    return fail(e, "out of memory");
  *p = q;
  return true;
}
bool jc_ttm_parse(const uint8_t *d, size_t n, size_t pages, JCTtm *t,
                  JCError *e) {
  size_t p = 0;
  memset(t, 0, sizeof *t);
  if (!pages)
    return true;
  if (pages > 65535)
    return fail(e, "too many TTM pages");
  t->frames = calloc(pages, sizeof *t->frames);
  if (!t->frames)
    return fail(e, "out of memory");
  t->frame_count = 1;
  while (span(n, p, 2)) {
    JCOp op = {0};
    JCFrame *f;
    size_t j;
    unsigned nargs;
    op.code = u16(d + p);
    p += 2;
    nargs = op.code & 15;
    if (nargs == 15) {
      size_t z = zero_length(d + p, n - p), consumed = z + 1 + (z % 2 == 0);
      if (z == n - p || !span(n, p, consumed))
        goto bad;
      op.text = text_copy(d + p, z);
      if (!op.text)
        goto bad;
      p += consumed;
    } else {
      if (nargs > 6 || !span(n, p, nargs * 2))
        goto bad;
      for (j = 0; j < nargs; j++)
        op.args[j] = (int16_t)u16(d + p + j * 2);
      p += nargs * 2;
    }
    if (t->frame_count > pages) {
      free(op.text);
      continue;
    }
    f = &t->frames[t->frame_count - 1];
    if (!append((void **)&f->ops, f->count + 1, sizeof *f->ops, e)) {
      free(op.text);
      goto bad;
    }
    f->ops[f->count++] = op;
    if (op.code == 0x1111) {
      if (!append((void **)&t->scenes, t->scene_count + 1, sizeof *t->scenes,
                  e))
        goto bad;
      t->scenes[t->scene_count].tag = (uint16_t)op.args[0];
      t->scenes[t->scene_count++].frame = t->frame_count - 1;
    }
    if (op.code == 0x0FF0 && t->frame_count < pages)
      t->frame_count++;
    else if (op.code == 0x0FF0 && t->frame_count == pages)
      break;
  }
  if (t->frame_count && t->frames[t->frame_count - 1].count == 0)
    t->frame_count--;
  return true;
bad:
  jc_ttm_free(t);
  return fail(e, "invalid TTM instruction stream");
}
size_t jc_ttm_frame_of_tag(const JCTtm *t, int tag) {
  size_t i;
  for (i = 0; i < t->frame_count; i++)
    if (t->frames[i].count) {
      const JCOp *o = &t->frames[i].ops[0];
      if ((o->code == 0x1101 || o->code == 0x1111) && o->args[0] == tag)
        return i;
    }
  return SIZE_MAX;
}
bool jc_archive_ttm(const JCArchive *a, const char *n, JCTtm *t, JCError *e) {
  View pag, code;
  JCBlob b = {0};
  bool ok;
  memset(t, 0, sizeof *t);
  if (!chunk(a, n, "PAG:", &pag, e) || pag.n < 2 ||
      !chunk(a, n, "TT3:", &code, e))
    return fail(e, "%s: invalid TTM", n);
  if (!jc_decompress(code.d, code.n, &b, e))
    return false;
  ok = jc_ttm_parse(b.data, b.size, u16(pag.d), t, e);
  jc_blob_free(&b);
  return ok;
}
void jc_ads_free(JCAds *a) {
  size_t i;
  if (a) {
    for (i = 0; a->ttms && i < a->ttm_count; i++)
      free(a->ttms[i].name);
    free(a->ttms);
    jc_blob_free(&a->script);
    memset(a, 0, sizeof *a);
  }
}
bool jc_archive_ads(const JCArchive *a, const char *n, JCAds *out, JCError *e) {
  View res, scr;
  size_t i, p = 2;
  memset(out, 0, sizeof *out);
  if (!chunk(a, n, "RES:", &res, e) || res.n < 2 ||
      !chunk(a, n, "SCR:", &scr, e))
    return fail(e, "%s: invalid ADS", n);
  out->ttm_count = u16(res.d);
  out->ttms = calloc(out->ttm_count ? out->ttm_count : 1, sizeof *out->ttms);
  if (!out->ttms)
    goto bad;
  for (i = 0; i < out->ttm_count; i++) {
    size_t len;
    if (!span(res.n, p, 2))
      goto bad;
    out->ttms[i].id = u16(res.d + p);
    p += 2;
    len = zero_length(res.d + p, res.n - p);
    if (len == res.n - p)
      goto bad;
    out->ttms[i].name = text_copy(res.d + p, len);
    if (!out->ttms[i].name)
      goto bad;
    p += len + 1;
  }
  if (!jc_decompress(scr.d, scr.n, &out->script, e))
    goto bad;
  return true;
bad:
  jc_ads_free(out);
  return fail(e, "%s: invalid ADS resource list/script", n);
}

typedef struct {
  const uint8_t *d;
  size_t n, ne;
} NE;
static bool ne_parse(NE *ne, const uint8_t *d, size_t n, JCError *e) {
  uint32_t off;
  if (n < 64 || memcmp(d, "MZ", 2))
    return fail(e, "SCRANTIC is not MZ executable");
  off = u32(d + 0x3c);
  if (!span(n, off, 0x40) || memcmp(d + off, "NE", 2))
    return fail(e, "SCRANTIC is not Win16 NE executable");
  ne->d = d;
  ne->n = n;
  ne->ne = off;
  return true;
}
static bool segment(const NE *ne, unsigned idx, View *v, JCError *e) {
  size_t table = ne->ne + u16(ne->d + ne->ne + 0x22), off, len, entry;
  unsigned shift = u16(ne->d + ne->ne + 0x32),
           count = u16(ne->d + ne->ne + 0x1c);
  if (!idx || idx > count || shift > 15)
    return fail(e, "missing NE segment");
  entry = table + (idx - 1) * 8;
  if (!span(ne->n, entry, 8))
    return fail(e, "truncated NE segment table");
  off = (size_t)u16(ne->d + entry) << shift;
  len = u16(ne->d + entry + 2);
  if (!len)
    len = 65536;
  if (!span(ne->n, off, len))
    return fail(e, "truncated NE segment %u", idx);
  v->d = ne->d + off;
  v->n = len;
  return true;
}
void jc_exe_free(JCExe *x) {
  size_t i, j;
  if (!x)
    return;
  for (i = 0; i < 256; i++)
    free(x->ads_names[i]);
  free(x->scenes);
  for (i = 0; i < 35; i++)
    free(x->paths[i].frames);
  for (i = 0; i < 7; i++)
    for (j = 0; j < 7; j++)
      free(x->routes[i][j].words);
  for (i = 0; i < x->sound_count; i++)
    free(x->sounds[i].samples);
  free(x->sounds);
  memset(x, 0, sizeof *x);
}
typedef struct {
  uint16_t type, id;
  View bytes;
} NERes;
static bool sound_decode(const View *v, JCSound *s, JCError *e) {
  size_t p = 12;
  if (v->n < 12 || memcmp(v->d, "RIFF", 4) || memcmp(v->d + 8, "WAVE", 4))
    return fail(e, "invalid WAVE resource");
  while (span(v->n, p, 8)) {
    size_t z = u32(v->d + p + 4), i;
    if (!span(v->n, p + 8, z))
      return fail(e, "truncated WAVE chunk");
    if (!memcmp(v->d + p, "data", 4)) {
      s->samples = malloc((z ? z : 1) * sizeof(float));
      if (!s->samples)
        return fail(e, "out of memory");
      s->count = z;
      for (i = 0; i < z; i++)
        s->samples[i] = ((float)v->d[p + 8 + i] - 128.0f) / 128.0f;
      return true;
    }
    p += 8 + z + (z & 1);
  }
  return fail(e, "WAVE resource missing PCM");
}
static bool sounds(const NE *ne, JCExe *x, JCError *e) {
  size_t table = ne->ne + u16(ne->d + ne->ne + 0x24), p, count = 0, i;
  uint16_t shift;
  NERes *r = NULL;
  View names = {0};
  bool ok = false;
  if (!word(ne->d, ne->n, table, &shift, e) || shift > 15)
    return fail(e, "bad NE resource shift");
  p = table + 2;
  while (1) {
    uint16_t type, n;
    if (!word(ne->d, ne->n, p, &type, e))
      goto done;
    if (!type)
      break;
    if (!word(ne->d, ne->n, p + 2, &n, e) ||
        !span(ne->n, p, 8 + (size_t)n * 12))
      goto done;
    for (i = 0; i < n; i++) {
      size_t q = p + 8 + i * 12, off = (size_t)u16(ne->d + q) << shift,
             z = (size_t)u16(ne->d + q + 2) << shift;
      if (!span(ne->n, off, z))
        goto done;
      if (!append((void **)&r, count + 1, sizeof *r, e))
        goto done;
      r[count].type = type;
      r[count].id = u16(ne->d + q + 6);
      r[count].bytes.d = ne->d + off;
      r[count].bytes.n = z;
      if (type == 0x800f && r[count].id == 0x8001)
        names = r[count].bytes;
      count++;
    }
    p += 8 + (size_t)n * 12;
  }
  p = 0;
  while (span(names.n, p, 6)) {
    uint16_t len = u16(names.d + p), type, id;
    size_t j, first, second;
    if (!len)
      break;
    if (len < 6 || !span(names.n, p, len))
      goto done;
    type = u16(names.d + p + 2);
    id = u16(names.d + p + 4);
    first = zero_length(names.d + p + 6, len - 6);
    second = 6 + first + 1;
    if (second < len) {
      size_t z = zero_length(names.d + p + second, len - second);
      const uint8_t *name = names.d + p + second;
      if (z > 7 && !memcmp(name, "WAVESFX", 7)) {
        uint32_t num = 0;
        bool digits = true;
        for (j = 7; j < z; j++) {
          if (name[j] < '0' || name[j] > '9') {
            digits = false;
            break;
          }
          num = num * 10 + (name[j] - '0');
        }
        if (digits) {
          for (j = 0; j < count; j++)
            if (r[j].type == type && r[j].id == id) {
              JCSound s = {0};
              s.id = num;
              if (!sound_decode(&r[j].bytes, &s, e))
                goto done;
              if (!append((void **)&x->sounds, x->sound_count + 1,
                          sizeof *x->sounds, e)) {
                free(s.samples);
                goto done;
              }
              x->sounds[x->sound_count++] = s;
              break;
            }
        }
      }
    }
    p += len;
  }
  ok = true;
done:
  free(r);
  if (!ok)
    fail(e, "invalid NE sounds/name table");
  return ok;
}
bool jc_exe_parse(JCExe *x, const uint8_t *d, size_t n, JCError *e) {
  NE ne;
  View ds, wc;
  size_t p, i, j;
  uint16_t w;
  memset(x, 0, sizeof *x);
  if (!ne_parse(&ne, d, n, e) || !segment(&ne, 14, &ds, e) ||
      !segment(&ne, 4, &wc, e))
    goto bad;
  if (!span(ds.n, 0x1681, 12) || memcmp(ds.d + 0x1681, "ACTIVITY.ADS", 12) ||
      !span(ds.n, 0x1d56, 9) || memcmp(ds.d + 0x1d56, "WAVESFX%d", 9)) {
    fail(e, "unsupported SCRANTIC.SCR build (1992 release required)");
    goto bad;
  }
  p = 0x166e;
  while (1) {
    uint16_t id;
    if (!word(ds.d, ds.n, p, &id, e))
      goto bad;
    if (!id)
      break;
    if (!span(ds.n, p, 17))
      goto bad;
    if (id < 256) {
      free(x->ads_names[id]);
      x->ads_names[id] = text_copy(ds.d + p + 2, zero_length(ds.d + p + 2, 15));
      if (!x->ads_names[id])
        goto bad;
    }
    p += 17;
  }
  p = 0x1756;
  while (1) {
    JCSceneDef *s;
    const uint8_t *r;
    if (!word(ds.d, ds.n, p, &w, e))
      goto bad;
    if (!w)
      break;
    if (!span(ds.n, p, 17) ||
        !append((void **)&x->scenes, x->scene_count + 1, sizeof *x->scenes, e))
      goto bad;
    s = &x->scenes[x->scene_count++];
    r = ds.d + p;
    s->weight = r[2];
    s->spot_start = r[3];
    s->hdg_start = r[4];
    s->spot_end = r[5];
    s->hdg_end = r[6];
    s->duration = r[7];
    s->min_phase = r[8];
    s->max_phase = r[9];
    s->day = r[10];
    s->flags = u32(r + 11);
    s->ads_id = r[15];
    s->ads_tag = r[16];
    p += 17;
  }
  if (!span(ds.n, 0x2de, 35 * 3) || !span(ds.n, 0x347, 14))
    goto bad;
  memcpy(x->path_info, ds.d + 0x2de, 35 * 3);
  memcpy(x->stand_turns, ds.d + 0x347, 7);
  memcpy(x->walk_turns, ds.d + 0x34e, 7);
  for (i = 1; i <= 34; i++) {
    JCWalkPath *path = &x->paths[i];
    uint16_t handler;
    if (!word(wc.d, wc.n, 0x4fb + (i - 1) * 2, &handler, e) ||
        !span(wc.n, handler, 3) || wc.d[handler] != 0xb8)
      goto bad;
    p = u16(wc.d + handler + 1);
    while (1) {
      JCWalkFrame *f;
      int16_t xx, yy;
      if (!span(ds.n, p, 6))
        goto bad;
      w = u16(ds.d + p);
      xx = (int16_t)u16(ds.d + p + 2);
      yy = (int16_t)u16(ds.d + p + 4);
      if (!w && !xx && !yy)
        break;
      if (!append((void **)&path->frames, path->count + 1, sizeof *path->frames,
                  e))
        goto bad;
      f = &path->frames[path->count++];
      f->sprite = w & 0x7fff;
      f->mirror = (w & 0x8000) != 0;
      f->x = xx;
      f->y = yy;
      p += 6;
    }
  }
  for (i = 1; i <= 6; i++)
    for (j = 1; j <= 6; j++) {
      JCRoute *r = &x->routes[i][j];
      if (!word(ds.d, ds.n, 0xa96 - 2 + i * 12 + j * 2, &w, e))
        goto bad;
      if (!w)
        continue;
      p = w;
      while (1) {
        if (!word(ds.d, ds.n, p, &w, e) ||
            !append((void **)&r->words, r->count + 1, sizeof *r->words, e))
          goto bad;
        r->words[r->count++] = (int16_t)w;
        p += 2;
        if ((int16_t)w == -7)
          break;
      }
    }
  for (i = 0; i < 56; i++)
    if (!word(ds.d, ds.n, 0x1ce6 + i * 2, &x->rng_table[i], e))
      goto bad;
  if (!word(ds.d, ds.n, 0x1ce2, &w, e))
    goto bad;
  x->rng_j = w;
  if (!word(ds.d, ds.n, 0x1ce4, &w, e))
    goto bad;
  x->rng_k = w;
  if (x->rng_j >= 56 || x->rng_k >= 56) {
    fail(e, "invalid RNG indices");
    goto bad;
  }
  if (!span(ds.n, 0x268, 24))
    goto bad;
  for (i = 0; i < 3; i++)
    for (j = 0; j < 8 && ds.d[0x268 + i * 8 + j] != 0xff; j++) {
      x->whitecap_seqs[i][j] = ds.d[0x268 + i * 8 + j];
      x->whitecap_counts[i]++;
    }
  if (!sounds(&ne, x, e))
    goto bad;
  return true;
bad:
  jc_exe_free(x);
  return fail(e, "invalid or unsupported SCRANTIC NE tables");
}
static bool read_ci(const char *dir, const char *name, JCBlob *b, JCError *e) {
  DIR *dp = opendir(dir);
  struct dirent *de;
  char path[4096];
  FILE *f = NULL;
  long n;
  if (!dp)
    return fail(e, "cannot open asset directory %s", dir);
  while ((de = readdir(dp)))
    if (same_name(de->d_name, name)) {
      if (snprintf(path, sizeof path, "%s/%s", dir, de->d_name) >=
          (int)sizeof path)
        break;
      f = fopen(path, "rb");
      break;
    }
  closedir(dp);
  if (!f)
    return fail(e, "missing %s in %s", name, dir);
  if (fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 || n > 128 * 1024 * 1024L ||
      fseek(f, 0, SEEK_SET)) {
    fclose(f);
    return fail(e, "invalid file size: %s", name);
  }
  b->data = malloc(n ? (size_t)n : 1);
  if (!b->data) {
    fclose(f);
    return fail(e, "out of memory");
  }
  b->size = (size_t)n;
  if (fread(b->data, 1, b->size, f) != b->size) {
    fclose(f);
    jc_blob_free(b);
    return fail(e, "cannot read %s", name);
  }
  fclose(f);
  return true;
}
bool jc_assets_load(JCAssets *a, const char *dir, JCError *e) {
  JCBlob exe = {0}, map = {0}, vol = {0};
  bool ok = false;
  memset(a, 0, sizeof *a);
  if (e)
    e->message[0] = 0;
  if (!read_ci(dir, "SCRANTIC.SCR", &exe, e) ||
      !read_ci(dir, "RESOURCE.MAP", &map, e) ||
      !read_ci(dir, "RESOURCE.001", &vol, e))
    goto done;
  if (vol.size < 1024) {
    fail(e, "RESOURCE.001 is installer placeholder");
    goto done;
  }
  if (!jc_archive_parse(&a->archive, map.data, map.size, vol.data, vol.size,
                        e) ||
      !jc_exe_parse(&a->exe, exe.data, exe.size, e))
    goto done;
  ok = true;
done:
  jc_blob_free(&exe);
  jc_blob_free(&map);
  jc_blob_free(&vol);
  if (!ok)
    jc_assets_free(a);
  return ok;
}
void jc_assets_free(JCAssets *a) {
  if (a) {
    jc_archive_free(&a->archive);
    jc_exe_free(&a->exe);
  }
}
