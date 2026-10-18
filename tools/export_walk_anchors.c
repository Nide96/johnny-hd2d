/* Extract coordinates only; no original pixels are embedded in generated code.
 */
#include "resource.h"
#include <stdio.h>
int main(int argc, char **argv) {
  JCAssets a = {0};
  JCBmp b = {0};
  JCError e = {{0}};
  if (!jc_assets_load(&a, argc > 1 ? argv[1] : "assets/original", &e) ||
      !jc_archive_bmp(&a.archive, "JOHNWALK.BMP", &b, &e)) {
    fprintf(stderr, "%s\n", e.message);
    return 1;
  }
  puts("/* Generated from JOHNWALK by tools/export_walk_anchors.c. Coordinates "
       "only. */");
  puts("typedef struct {float width,pivot;} JCWalkAnchor;\nstatic const "
       "JCWalkAnchor walk_anchors[36]={");
  for (unsigned id = 0; id < 36; ++id) {
    const JCSprite *s = &b.sprites[id];
    int x1 = s->w, x2 = -1;
    for (int y = 0; y < s->h / 5; ++y)
      for (int x = 0; x < s->w; ++x)
        if (s->px[y * s->w + x]) {
          if (x < x1)
            x1 = x;
          if (x > x2)
            x2 = x;
        }
    if (x2 < x1) {
      x1 = 0;
      x2 = s->w - 1;
    }
    printf("{%d.f,%.1ff},\n", s->w, (x1 + x2 + 1) * .5f);
  }
  puts("};");
  jc_bmp_free(&b);
  jc_assets_free(&a);
  return 0;
}
