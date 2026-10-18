#include "../resource.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t hash_bytes(uint64_t h, const uint8_t *p, size_t n) {
  while (n--) {
    h ^= *p++;
    h *= UINT64_C(1099511628211);
  }
  return h;
}
static int suffix(const char *n, const char *s) {
  size_t a = strlen(n), b = strlen(s);
  return a >= b && !strcmp(n + a - b, s);
}
int main(int argc, char **argv) {
  JCAssets a = {0};
  JCError e = {{0}};
  JCBmp bmp = {0};
  JCPicture pic = {0};
  JCTtm ttm = {0};
  JCBlob blob = {0};
  JCExe exe = {0};
  JCArchive archive = {0};
  size_t i, j, k, nbmp = 0, nscr = 0, nttm = 0, nads = 0, npal = 0,
                  nsprites = 0, nframes = 0, nops = 0;
  bool opcodes[65536] = {0};
  uint64_t hash = UINT64_C(14695981039346656037);
  uint8_t invalid_lzw[] = {2, 8, 0, 0, 0, 255, 255, 255, 255},
          huge[] = {2, 255, 255, 255, 127}, unknown[] = {9, 0, 0, 0, 0},
          map[21] = {0};
  assert(!jc_exe_parse(&exe, NULL, 0, &e));
  assert(!jc_exe_parse(&exe, map, sizeof map, &e));
  assert(!jc_decompress(invalid_lzw, sizeof invalid_lzw, &blob, &e));
  assert(!jc_decompress(huge, sizeof huge, &blob, &e));
  assert(!jc_decompress(unknown, sizeof unknown, &blob, &e));
  map[19] = 255;
  map[20] = 255;
  assert(!jc_archive_parse(&archive, map, sizeof map, map, sizeof map, &e));
  {
    uint8_t ok[] = {1, 4, 0, 0, 0, 0x84, 0x2a};
    assert(jc_decompress(ok, sizeof ok, &blob, &e));
    assert(blob.size == 4 && blob.data[3] == 42);
    jc_blob_free(&blob);
  }
  {
    uint8_t no[] = {0x2f, 0x10, 'x'};
    assert(!jc_ttm_parse(no, sizeof no, 1, &ttm, &e));
  }
  if (!jc_assets_load(&a, argc > 1 ? argv[1] : "assets/original", &e)) {
    fprintf(stderr, "load: %s\n", e.message);
    return 1;
  }
  assert(a.exe.scene_count == 80);
  assert(a.exe.sound_count == 23);
  assert(jc_archive_bmp(&a.archive, "JOHNWALK.BMP", &bmp, &e));
  assert(bmp.count == 36);
  jc_bmp_free(&bmp);
  assert(jc_archive_ttm(&a.archive, "FISHWALK.TTM", &ttm, &e));
  assert(ttm.scene_count == 1 && ttm.scenes[0].tag == 19 &&
         ttm.scenes[0].frame == 1);
  jc_ttm_free(&ttm);
  assert(jc_archive_scr(&a.archive, "OCEAN00.SCR", &pic, &e));
  assert(pic.w == 640 && pic.h == 480);
  jc_picture_free(&pic);
  for (i = 0; i < a.archive.count; i++) {
    const char *n = a.archive.files[i].name;
    if (suffix(n, ".BMP")) {
      assert(jc_archive_bmp(&a.archive, n, &bmp, &e));
      nbmp++;
      nsprites += bmp.count;
      for (j = 0; j < bmp.count; j++)
        hash = hash_bytes(hash, bmp.sprites[j].px,
                          (size_t)bmp.sprites[j].w * (size_t)bmp.sprites[j].h);
      jc_bmp_free(&bmp);
    } else if (suffix(n, ".SCR")) {
      assert(jc_archive_scr(&a.archive, n, &pic, &e));
      nscr++;
      hash = hash_bytes(hash, pic.px, (size_t)pic.w * (size_t)pic.h);
      jc_picture_free(&pic);
    } else if (suffix(n, ".TTM")) {
      if (!jc_archive_ttm(&a.archive, n, &ttm, &e)) {
        fprintf(stderr, "%s: %s\n", n, e.message);
        abort();
      }
      nttm++;
      nframes += ttm.frame_count;
      for (j = 0; j < ttm.frame_count; j++)
        for (k = 0; k < ttm.frames[j].count; k++) {
          opcodes[ttm.frames[j].ops[k].code] = true;
          nops++;
        }
      jc_ttm_free(&ttm);
    } else if (suffix(n, ".ADS")) {
      JCAds ads = {0};
      assert(jc_archive_ads(&a.archive, n, &ads, &e));
      nads++;
      hash = hash_bytes(hash, ads.script.data, ads.script.size);
      jc_ads_free(&ads);
    } else if (suffix(n, ".PAL")) {
      uint8_t pal[256][3];
      assert(jc_archive_palette(&a.archive, n, pal, &e));
      npal++;
      hash = hash_bytes(hash, &pal[0][0], sizeof pal);
    }
  }
  printf("resources=%zu scenes=%zu sounds=%zu paths=35 BMP=%zu sprites=%zu "
         "SCR=%zu PAL=%zu TTM=%zu frames=%zu ops=%zu ADS=%zu FNV64=%016llx\n",
         a.archive.count, a.exe.scene_count, a.exe.sound_count, nbmp, nsprites,
         nscr, npal, nttm, nframes, nops, nads, (unsigned long long)hash);
  printf("TTM opcodes:");
  for (i = 0; i < 65536; i++)
    if (opcodes[i])
      printf(" %04zx", i);
  puts("");
  /* Actual executable and MAP truncations must fail without invalid access. */
  {
    const char *dir = argc > 1 ? argv[1] : "assets/original";
    char path[4096];
    const char *names[] = {"RESOURCE.MAP", "SCRANTIC.SCR"};
    size_t which;
    for (which = 0; which < 2; which++) {
      FILE *f;
      long length;
      uint8_t *bytes;
      snprintf(path, sizeof path, "%s/%s", dir, names[which]);
      f = fopen(path, "rb");
      assert(f && !fseek(f, 0, SEEK_END));
      length = ftell(f);
      assert(length > 0 && !fseek(f, 0, SEEK_SET));
      bytes = malloc((size_t)length);
      assert(bytes && fread(bytes, 1, (size_t)length, f) == (size_t)length);
      fclose(f);
      for (i = 0; i < (size_t)length; i += which ? 997 : 1) {
        if (which) {
          (void)jc_exe_parse(&exe, bytes, i, &e);
          jc_exe_free(&exe);
        } else {
          assert(!jc_archive_parse(&archive, bytes, i, a.archive.volume.data,
                                   a.archive.volume.size, &e));
          jc_archive_free(&archive);
        }
      }
      free(bytes);
    }
  }
  /* Exercise invalid BMP cleanup before sprites have been allocated. */
  {
    uint8_t bad[] = {'I', 'N', 'F', ':', 2, 0, 0, 0, 255, 255, 'B', 'I',
                     'N', ':', 5,   0,   0, 0, 1, 0, 0,   0,   0};
    JCResource entry = {"BAD.BMP", bad, sizeof bad};
    JCArchive fake = {&entry, 1, {NULL, 0}};
    assert(!jc_archive_bmp(&fake, "BAD.BMP", &bmp, &e));
  }
  /* Small deterministic corrupt compressed streams (bounded output). */
  {
    uint32_t rng = 0x12345678u;
    uint8_t data[80];
    size_t trial;
    for (trial = 0; trial < 5000; trial++) {
      for (i = 0; i < sizeof data; i++) {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        data[i] = (uint8_t)rng;
      }
      data[0] = (trial & 1) ? 1 : 2;
      data[1] = 32;
      data[2] = data[3] = data[4] = 0;
      (void)jc_decompress(data, sizeof data, &blob, &e);
      jc_blob_free(&blob);
    }
  }
  /* Many volume truncations must fail safely. */
  for (i = 0; i < a.archive.volume.size; i += 4999) {
    uint8_t one_map[29] = {0};
    one_map[19] = 1;
    one_map[25] = (uint8_t)(i & 255);
    one_map[26] = (uint8_t)((i >> 8) & 255);
    one_map[27] = (uint8_t)((i >> 16) & 255);
    (void)jc_archive_parse(&archive, one_map, sizeof one_map,
                           a.archive.volume.data, i, &e);
    jc_archive_free(&archive);
  }
  jc_assets_free(&a);
  puts("resource tests: OK");
  return 0;
}
