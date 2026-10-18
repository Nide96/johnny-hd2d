#ifndef JC_RENDERER_VULKAN_H
#define JC_RENDERER_VULKAN_H
#include "scene.h"
#ifndef SOKOL_GFX_INCLUDED
#include "sokol_gfx.h"
#endif
typedef struct Renderer Renderer;
Renderer *renderer_create(const uint8_t palette[256][3],
                          const char *modern_plate, const char *palm_sheet);
bool renderer_frame(Renderer *, const SceneState *, sg_swapchain,
                    bool interpolate);
void renderer_destroy(Renderer *);
void renderer_set_hour(Renderer *, double hour);
void renderer_set_environment(Renderer *, double seconds, bool enabled);
#endif
