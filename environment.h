#ifndef JC_ENVIRONMENT_H
#define JC_ENVIRONMENT_H
#include "render_frame.h"
enum { JC_ENV_W = 640, JC_ENV_H = 360 };
typedef struct {
  float top[4], horizon[4], sun[4], moon[4], factors[4];
} JCAtmosphere;
JCAtmosphere jc_atmosphere(double hour);
bool jc_environment_wind(unsigned scene, bool walk_preview);
/* Runtime material mask: R signed shore distance (+/-32 world pixels),
 * G land coverage, B rock exclusion. No authored replacement bitmap. */
uint8_t *jc_environment_mask(const uint8_t *rgba, int width, int height);
/* x/y in the shared 1280x720 world, intensity, radius. Zero unless a known
 * fire resource contains visible yellow flame pixels. */
void jc_environment_fire(const JCRenderFrame *, float out[4]);
#endif
