#define _POSIX_C_SOURCE 200809L
#include "../config.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define SECTION "[ScreenSaver.ScreenAntics]\n"
static void write_text(const char *path, const char *text) {
  FILE *f = fopen(path, "wb");
  assert(f);
  assert(fwrite(text, 1, strlen(text), f) == strlen(text));
  assert(!fclose(f));
}
static void defaults(const JCConfig *c) {
  assert(c->background && c->clouds && c->waves && c->sounds);
  assert(c->start_time == 900 && c->intro == 0 && c->num_days == 0 &&
         c->intro_saved == 0);
}
int main(int argc, char **argv) {
  JCConfig c, d;
  JCError e = {{0}};
  char root[4096], path[4096], badpath[4096], text[1024];
  DIR *directory;
  struct dirent *entry;
  size_t i, count = 0;
  const char *invalid[] = {"Sounds=2",        "Sounds=abc",
                           "Sounds=",         "NumDays=12",
                           "Introduction=-1", "StartTime=960",
                           "StartTime=2400",  "CurrentMonth=13",
                           "garbage",         "CurrentYear=10000",
                           "CurrentDay=32",   "Waves=1=1",
                           "Background=0x1",  "StartTime=2147483648",
                           "Sounds=1.0"};
  assert(jc_config_parse("", &c));
  defaults(&c);
  assert(jc_config_parse(
      SECTION
      "Background=1\nClouds=0\nStartTime=2359\nNumDays=11\nIntroduction=12\n",
      &c));
  assert(c.background && !c.clouds && c.start_time == 2359 &&
         c.num_days == 11 && c.intro == 12);
  for (i = 0; i < sizeof invalid / sizeof *invalid; i++) {
    snprintf(text, sizeof text, SECTION "%s\n", invalid[i]);
    assert(!jc_config_parse(text, &c));
    defaults(&c);
  }
  assert(
      jc_config_parse("[Other]\nSounds=9\n" SECTION "Foo=bar\nSounds=0\n", &c));
  assert(!c.sounds);
  assert(jc_config_parse(
      "  [screensaver.screenantics] \r\n sounds = +0 \r\nClouds = "
      "-0\r\nWaves=0000000000000000000000000000000000000000001\nWaves=0\n "
      "[Other]\n garbage\n",
      &c));
  assert(!c.sounds && !c.clouds && !c.waves);
  assert(!jc_config_parse(NULL, &c));
  defaults(&c);
  assert(!jc_config_parse("", NULL));
  assert(snprintf(root, sizeof root, "%s/config-test-XXXXXX",
                  argc > 1 ? argv[1] : "work") < (int)sizeof root);
  assert(mkdtemp(root));
  assert(snprintf(path, sizeof path, "%s/SCRANTIC.INI", root) <
         (int)sizeof path);
  assert(snprintf(badpath, sizeof badpath, "%s/missing/config.ini", root) <
         (int)sizeof badpath);
  assert(jc_config_load(path, &c, &e));
  defaults(&c);
  assert(access(path, F_OK));
  c.num_days = 7;
  c.intro = 3;
  c.intro_saved = 6;
  c.month = 10;
  c.day = 18;
  c.year = 2026;
  c.clouds = false;
  c.sounds = false;
  assert(jc_config_save(path, &c, &e));
  assert(jc_config_load(path, &d, &e));
  assert(d.num_days == 7 && d.intro == 6 && d.intro_saved == 0 && !d.clouds &&
         !d.sounds && d.year == 2026 && d.month == 10 && d.day == 18);
  c.intro_saved = 0;
  c.intro = 8;
  assert(jc_config_save(path, &c, &e));
  assert(jc_config_load(path, &d, &e));
  assert(d.intro == 8);
  c.start_time = 960;
  assert(!jc_config_save(path, &c, &e));
  assert(jc_config_load(path, &d, &e) && d.start_time == 900);
  assert(!jc_config_save(badpath, &d, &e));
  assert(e.message[0]);
  write_text(path, SECTION "Sounds=9\n");
  assert(!jc_config_load(path, &d, &e));
  defaults(&d);
  assert(e.message[0]);
  {
    FILE *f = fopen(path, "wb");
    const char bad[] = SECTION "Sounds=0\0NumDays=9";
    assert(f);
    assert(fwrite(bad, 1, sizeof bad - 1, f) == sizeof bad - 1);
    assert(!fclose(f));
    assert(!jc_config_load(path, &d, &e));
    defaults(&d);
  }
  directory = opendir(root);
  assert(directory);
  while ((entry = readdir(directory)))
    if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
      assert(!strcmp(entry->d_name, "SCRANTIC.INI"));
      count++;
    }
  assert(!closedir(directory) && count == 1);
  assert(!unlink(path) && !rmdir(root));
  puts("config tests: OK (strict original keys, defaults, atomic round trips)");
  return 0;
}
