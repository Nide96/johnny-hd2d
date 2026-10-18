#ifndef JC_RESOURCE_H
#define JC_RESOURCE_H
/* Clean C99 port of open-scrantic assets (MIT), Glauber Lima and contributors.
 * Data are owned by the returned object. Zero-initialize before load; free even
 * after a failed load. Archive entries are views into archive.volume. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define JC_ASSET_MAX_UNPACKED (16u << 20)
typedef struct {
  char message[256];
} JCError;
typedef struct {
  uint8_t *data;
  size_t size;
} JCBlob;
typedef struct {
  char name[14];
  const uint8_t *data;
  size_t size;
} JCResource;
typedef struct {
  JCResource *files;
  size_t count;
  JCBlob volume;
} JCArchive;
typedef struct {
  int w, h;
  uint8_t *px;
} JCSprite;
typedef struct {
  JCSprite *sprites;
  size_t count;
} JCBmp;
typedef JCSprite JCPicture;
typedef struct {
  uint16_t code;
  int16_t args[6];
  char *text;
} JCOp;
typedef struct {
  JCOp *ops;
  size_t count;
} JCFrame;
typedef struct {
  uint16_t tag;
  size_t frame;
} JCSceneTag;
typedef struct {
  JCFrame *frames;
  size_t frame_count;
  JCSceneTag *scenes;
  size_t scene_count;
} JCTtm;
typedef struct {
  uint16_t id;
  char *name;
} JCTtmRef;
typedef struct {
  JCTtmRef *ttms;
  size_t ttm_count;
  JCBlob script;
} JCAds;
typedef struct {
  uint8_t weight, spot_start, hdg_start, spot_end, hdg_end, duration, min_phase,
      max_phase, day;
  uint32_t flags;
  uint8_t ads_id, ads_tag;
} JCSceneDef;
typedef struct {
  uint16_t sprite;
  bool mirror;
  int32_t x, y;
} JCWalkFrame;
typedef struct {
  JCWalkFrame *frames;
  size_t count;
} JCWalkPath;
typedef struct {
  int16_t *words;
  size_t count;
} JCRoute;
typedef struct {
  uint32_t id;
  float *samples;
  size_t count;
} JCSound;
typedef struct {
  char *ads_names[256];
  JCSceneDef *scenes;
  size_t scene_count;
  uint8_t path_info[35][3], stand_turns[7], walk_turns[7];
  JCWalkPath paths[35];
  JCRoute routes[7][7];
  uint16_t rng_table[56];
  size_t rng_j, rng_k;
  uint8_t whitecap_seqs[3][8];
  size_t whitecap_counts[3];
  JCSound *sounds;
  size_t sound_count;
} JCExe;
typedef struct {
  JCArchive archive;
  JCExe exe;
} JCAssets;
bool jc_assets_load(JCAssets *, const char *dir, JCError *);
void jc_assets_free(JCAssets *);
bool jc_archive_parse(JCArchive *, const uint8_t *map, size_t map_size,
                      const uint8_t *volume, size_t volume_size, JCError *);
void jc_archive_free(JCArchive *);
const JCResource *jc_archive_find(const JCArchive *, const char *name);
bool jc_decompress(const uint8_t *, size_t, JCBlob *, JCError *);
void jc_blob_free(JCBlob *);
bool jc_archive_bmp(const JCArchive *, const char *, JCBmp *, JCError *);
void jc_bmp_free(JCBmp *);
bool jc_archive_scr(const JCArchive *, const char *, JCPicture *, JCError *);
void jc_picture_free(JCPicture *);
bool jc_archive_palette(const JCArchive *, const char *,
                        uint8_t palette[256][3], JCError *);
bool jc_archive_ttm(const JCArchive *, const char *, JCTtm *, JCError *);
bool jc_ttm_parse(const uint8_t *, size_t, size_t pages, JCTtm *, JCError *);
void jc_ttm_free(JCTtm *);
/* SIZE_MAX means no matching first-in-frame local tag. */
size_t jc_ttm_frame_of_tag(const JCTtm *, int tag);
bool jc_archive_ads(const JCArchive *, const char *, JCAds *, JCError *);
void jc_ads_free(JCAds *);
bool jc_exe_parse(JCExe *, const uint8_t *, size_t, JCError *);
void jc_exe_free(JCExe *);
#endif
