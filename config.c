/* C99 adaptation of engine/config.rs from open-scrantic.
 * Copyright (c) 2026 Glauber Lima. MIT License; see licenses/open-scrantic-MIT.txt. */
#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define SECTION "[ScreenSaver.ScreenAntics]"
#define MAX_CONFIG_BYTES (1024u * 1024u)
typedef struct {
  const char *p;
  size_t n;
} Slice;
static Slice trim(Slice s) {
  while (s.n && isspace((unsigned char)*s.p)) {
    ++s.p;
    --s.n;
  }
  while (s.n && isspace((unsigned char)s.p[s.n - 1]))
    --s.n;
  return s;
}
static bool equals(Slice s, const char *word) {
  size_t i, n = strlen(word);
  if (s.n != n)
    return false;
  for (i = 0; i < n; i++)
    if (tolower((unsigned char)s.p[i]) != tolower((unsigned char)word[i]))
      return false;
  return true;
}
static bool number(Slice s, int32_t high, int32_t *out) {
  size_t i = 0;
  uint32_t value = 0;
  bool negative = false;
  s = trim(s);
  if (!s.n)
    return false;
  if (s.p[0] == '+' || s.p[0] == '-') {
    negative = s.p[0] == '-';
    if (++i == s.n)
      return false;
  }
  for (; i < s.n; i++) {
    unsigned digit = (unsigned char)s.p[i] - '0';
    if (digit > 9 || value > (uint32_t)high / 10 ||
        value * 10 + digit > (uint32_t)high)
      return false;
    value = value * 10 + digit;
  }
  if (negative && value)
    return false;
  *out = (int32_t)value;
  return true;
}
static bool flag(Slice s, bool *out) {
  int32_t v;
  if (!number(s, 1, &v))
    return false;
  *out = v != 0;
  return true;
}
static bool apply(JCConfig *c, Slice key, Slice value) {
  key = trim(key);
  if (equals(key, "background"))
    return flag(value, &c->background);
  if (equals(key, "clouds"))
    return flag(value, &c->clouds);
  if (equals(key, "waves"))
    return flag(value, &c->waves);
  if (equals(key, "sounds"))
    return flag(value, &c->sounds);
  if (equals(key, "numdays"))
    return number(value, 11, &c->num_days);
  if (equals(key, "introduction"))
    return number(value, 12, &c->intro);
  if (equals(key, "currentyear"))
    return number(value, 9999, &c->year);
  if (equals(key, "currentmonth"))
    return number(value, 12, &c->month);
  if (equals(key, "currentday"))
    return number(value, 31, &c->day);
  if (equals(key, "starttime"))
    return number(value, 2359, &c->start_time) && c->start_time % 100 < 60;
  return true;
}
bool jc_config_parse(const char *text, JCConfig *out) {
  JCConfig c = jc_config_default();
  const char *p = text;
  bool section = false;
  size_t total;
  if (!out)
    return false;
  *out = c;
  if (!text)
    return false;
  total = strlen(text);
  if (total > MAX_CONFIG_BYTES)
    return false;
  while (*p) {
    const char *end = strchr(p, '\n');
    Slice line;
    if (!end)
      end = p + strlen(p);
    line = trim((Slice){p, (size_t)(end - p)});
    if (line.n) {
      if (line.p[0] == '[')
        section = equals(line, SECTION);
      else if (section) {
        const char *equal = memchr(line.p, '=', line.n);
        if (!equal ||
            !apply(&c, (Slice){line.p, (size_t)(equal - line.p)},
                   (Slice){equal + 1, line.n - (size_t)(equal + 1 - line.p)}))
          return false;
      }
    }
    p = *end ? end + 1 : end;
  }
  *out = c;
  return true;
}
static bool error(JCError *e, const char *message, const char *path) {
  if (e)
    snprintf(e->message, sizeof e->message, "%s: %s", message,
             path ? path : "(null)");
  return false;
}
bool jc_config_load(const char *path, JCConfig *out, JCError *e) {
  FILE *f;
  long size;
  char *text;
  bool ok;
  if (e)
    e->message[0] = 0;
  if (!out)
    return error(e, "null output config", path);
  *out = jc_config_default();
  if (!path)
    return error(e, "null config path", path);
  f = fopen(path, "rb");
  if (!f) {
    if (errno == ENOENT)
      return true;
    return error(e, "cannot read config", path);
  }
  if (fseek(f, 0, SEEK_END) || (size = ftell(f)) < 0 ||
      (unsigned long)size > MAX_CONFIG_BYTES || fseek(f, 0, SEEK_SET)) {
    fclose(f);
    return error(e, "invalid config file size", path);
  }
  text = malloc((size_t)size + 1);
  if (!text) {
    fclose(f);
    return error(e, "config allocation failed", path);
  }
  if (fread(text, 1, (size_t)size, f) != (size_t)size) {
    free(text);
    fclose(f);
    return error(e, "cannot read config", path);
  }
  text[size] = 0;
  if (fclose(f)) {
    free(text);
    return error(e, "cannot close config", path);
  }
  ok = memchr(text, 0, (size_t)size) == NULL && jc_config_parse(text, out);
  free(text);
  return ok ? true : error(e, "malformed config", path);
}
static bool valid_config(const JCConfig *c) {
  int32_t intro = c->intro_saved ? c->intro_saved : c->intro;
  return c->num_days >= 0 && c->num_days <= 11 && intro >= 0 && intro <= 12 &&
         c->year >= 0 && c->year <= 9999 && c->month >= 0 && c->month <= 12 &&
         c->day >= 0 && c->day <= 31 && c->start_time >= 0 &&
         c->start_time <= 2359 && c->start_time % 100 < 60;
}
bool jc_config_save(const char *path, const JCConfig *c, JCError *e) {
  size_t length;
  char *temp;
  int fd;
  FILE *f;
  bool ok;
  int intro;
  if (e)
    e->message[0] = 0;
  if (!path || !*path || !c || !valid_config(c))
    return error(e, "invalid config/path", path);
  length = strlen(path);
  if (length > SIZE_MAX - 12)
    return error(e, "config path too long", path);
  temp = malloc(length + 12);
  if (!temp)
    return error(e, "config allocation failed", path);
  memcpy(temp, path, length);
  memcpy(temp + length, ".tmp.XXXXXX", 12);
  fd = mkstemp(temp);
  if (fd < 0) {
    free(temp);
    return error(e, "cannot create temporary config", path);
  }
  f = fdopen(fd, "wb");
  if (!f) {
    close(fd);
    unlink(temp);
    free(temp);
    return error(e, "cannot open temporary config", path);
  }
  intro = c->intro_saved ? c->intro_saved : c->intro;
  ok = fprintf(f,
               SECTION "\nBackground=%d\nClouds=%d\nWaves=%d\nSounds=%"
                       "d\nNumDays=%d\nIntroduction=%d\nCurrentYear=%"
                       "d\nCurrentMonth=%d\nCurrentDay=%d\nStartTime=%d\n",
               c->background ? 1 : 0, c->clouds ? 1 : 0, c->waves ? 1 : 0,
               c->sounds ? 1 : 0, c->num_days, intro, c->year, c->month, c->day,
               c->start_time) >= 0;
  if (ok && fflush(f))
    ok = false;
  if (ok && fsync(fd))
    ok = false;
  if (fclose(f))
    ok = false;
  if (ok && rename(temp, path))
    ok = false;
  if (!ok)
    unlink(temp);
  free(temp);
  return ok ? true : error(e, "cannot atomically write config", path);
}
