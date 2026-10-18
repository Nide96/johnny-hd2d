#ifndef JC_AUDIO_H
#define JC_AUDIO_H
#include "resource.h"
typedef struct JCAudio JCAudio;
/* Device failure is nonfatal: returns NULL and leaves graphics available. */
JCAudio *jc_audio_create(const JCExe *, unsigned volume, bool muted);
void jc_audio_events(JCAudio *, const uint32_t *, size_t, bool enabled);
void jc_audio_toggle_mute(JCAudio *);
void jc_audio_destroy(JCAudio *);
#endif
