/* Window and command-line front end. The engine never calls Sokol. */
#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
#ifndef SOKOL_VULKAN
#define SOKOL_VULKAN
#endif
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_time.h"
#include "renderer_vulkan.h"
#include "vm_core.h"
#include "config.h"
#include "audio.h"
#include "audio_mixer.h"
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "hybrid.h"

typedef struct {
  char default_dir[4096], config_path[4096], modern_path[4096];
  const char *data_dir, *snapshot;
  uint64_t seed, steps;
  unsigned skip_steps;
  unsigned smoke_frames;
  unsigned volume;
  unsigned time_lapse, decor_time;
  bool no_decor_fx, freeze_scene, fixed_decor;
  bool mute, audio_test;
  int scene, story_day;
  bool fullscreen, modern, johnny_hd, walk_demo, verify, headless, list,
      no_interpolation, fixed_clock;
  JCClock clock;
} Options;
static struct {
  Options options;
  VmCore vm;
  Renderer *renderer;
  JCAudio *audio;
  uint64_t last_time, frames;
  double intro_left;
  double visual_seconds;
  int exit_code;
  bool ready, persist;
} app;

/* Screen-saver launchers commonly stop the child with SIGTERM. The handler
 * only sets a flag; all window, GPU and INI operations remain on the main loop.
 */
static volatile sig_atomic_t stop_requested;
static void stop_signal(int signal_number) {
  (void)signal_number;
  stop_requested = 1;
}

static JCClock wall_clock(void) {
  time_t now = time(NULL);
  struct tm local;
  if (!localtime_r(&now, &local))
    return (JCClock){2026, 10, 18, 12, 0};
  return (JCClock){local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                   local.tm_hour, local.tm_min};
}
static bool number(const char *text, uint64_t *value) {
  char *end = NULL;
  errno = 0;
  if (!text[0] || text[0] == '-')
    return false;
  unsigned long long n = strtoull(text, &end, 10);
  if (errno || *end)
    return false;
  *value = (uint64_t)n;
  return true;
}
static void usage(void) {
  puts("Johnny Castaway C99 / Sokol Vulkan\n"
       "  --data DIR            Original RESOURCE.MAP/.001 and SCRANTIC.SCR\n"
       "  --modern              Living 16:9 island, water, sky and lighting\n"
       "  --johnny-hd           Legacy alias; original R8 Johnny is always "
       "used\n"
       "  --walk-demo           Isolated original-path walk preview (implies "
       "modern)\n"
       "  --modern-plate FILE   Override JCR1 background texture\n"
       "  --palm-original       Original palm (default; compatibility option)\n"
       "  --no-decor-fx         Compare with the static illustrated sky\n"
       "  --time-lapse N        Visual 24-hour cycle in N seconds (24..3600)\n"
       "  --freeze-scene        Hold the VM while the decor keeps animating\n"
       "  --decor-time N        Fixed decor time for reproducible previews\n"
       "  --classic             Original indexed presentation (default)\n"
       "  --fullscreen          Fullscreen window\n"
       "  --mute                Start muted (M toggles sound)\n"
       "  --volume N            Effect volume 0..100 (default 70)\n"
       "  --audio-test          Play three diagnostic tones without graphics\n"
       "  --seed N              Deterministic original RNG warmup\n"
       "  --clock YYYY-MM-DDTHH:MM  Inject a fixed local calendar\n"
       "  --story-day N         Story chapter 1..11\n"
       "  --list-scenes         Print the original scene catalog\n"
       "  --scene N             Force one catalog scene (1..79)\n"
       "  --skip-steps N        Advance forced scene before preview (0..6000)\n"
       "  --no-interpolation    Present original discrete sprite positions\n"
       "  --headless N          Run N scheduler steps without graphics\n"
       "  --verify              Compare GPU command capture with CPU pixels\n"
       "  --snapshot FILE.ppm   Write final original-resolution image\n"
       "  --smoke               Render 240 window frames then exit\n"
       "  --help                This text");
}
static bool parse_options(int argc, char **argv, Options *o) {
  memset(o, 0, sizeof(*o));
  o->seed = (uint64_t)time(NULL);
  o->scene = -1;
  o->volume = 70;
  o->story_day = 0;
  strcpy(o->config_path, "SCRANTIC.INI");
  o->clock = wall_clock();
  ssize_t n =
      readlink("/proc/self/exe", o->default_dir, sizeof(o->default_dir) - 32);
  if (n > 0) {
    o->default_dir[n] = 0;
    char *slash = strrchr(o->default_dir, '/');
    if (slash) {
      strcpy(slash, "/SCRANTIC.INI");
      strcpy(o->config_path, o->default_dir);
      strcpy(slash, "/assets/original");
    } else
      strcpy(o->default_dir, "assets/original");
  } else
    strcpy(o->default_dir, "assets/original");
  o->data_dir = o->default_dir;
  snprintf(o->modern_path, sizeof o->modern_path,
           "%.*smodern/island-registered-v2.jcr",
           (int)(strlen(o->default_dir) - strlen("original")), o->default_dir);
  for (int i = 1; i < argc; ++i) {
    const char *a = argv[i];
    uint64_t value;
    if (!strcmp(a, "--help")) {
      usage();
      exit(0);
    }
    if (!strcmp(a, "--modern") || !strcmp(a, "--classic")) {
      o->modern = !strcmp(a, "--modern");
      continue;
    }
    if (!strcmp(a, "--johnny-hd")) {
      o->johnny_hd = true;
      continue;
    }
    if (!strcmp(a, "--walk-demo")) {
      o->walk_demo = true;
      continue;
    }
    if (!strcmp(a, "--fullscreen")) {
      o->fullscreen = true;
      continue;
    }
    if (!strcmp(a, "--mute")) {
      o->mute = true;
      continue;
    }
    if (!strcmp(a, "--audio-test")) {
      o->audio_test = true;
      continue;
    }
    if (!strcmp(a, "--palm-original")) {
      /* Accepted for old launchers: the original palm is now mandatory. */
      continue;
    }
    if (!strcmp(a, "--no-decor-fx")) {
      o->no_decor_fx = true;
      continue;
    }
    if (!strcmp(a, "--freeze-scene")) {
      o->freeze_scene = true;
      continue;
    }
    if (!strcmp(a, "--verify")) {
      o->verify = true;
      continue;
    }
    if (!strcmp(a, "--list-scenes")) {
      o->list = true;
      continue;
    }
    if (!strcmp(a, "--no-interpolation")) {
      o->no_interpolation = true;
      continue;
    }
    if (!strcmp(a, "--smoke")) {
      o->smoke_frames = 240;
      continue;
    }
    if (i + 1 >= argc) {
      fprintf(stderr, "Missing argument for %s\n", a);
      return false;
    }
    const char *v = argv[++i];
    if (!strcmp(a, "--modern-plate")) {
      if (strlen(v) >= sizeof o->modern_path)
        return false;
      strcpy(o->modern_path, v);
      continue;
    }
    if (!strcmp(a, "--data")) {
      o->data_dir = v;
      continue;
    }
    if (!strcmp(a, "--snapshot")) {
      o->snapshot = v;
      continue;
    }
    if (!strcmp(a, "--clock")) {
      char extra;
      if (sscanf(v, "%d-%d-%dT%d:%d%c", &o->clock.year, &o->clock.month,
                 &o->clock.day, &o->clock.hour, &o->clock.minute,
                 &extra) != 5 ||
          o->clock.month < 1 || o->clock.month > 12 || o->clock.day < 1 ||
          o->clock.day > 31 || o->clock.hour < 0 || o->clock.hour > 23 ||
          o->clock.minute < 0 || o->clock.minute > 59) {
        fputs("Invalid --clock\n", stderr);
        return false;
      }
      o->fixed_clock = true;
      continue;
    }
    if (!number(v, &value)) {
      fprintf(stderr, "Invalid number: %s\n", v);
      return false;
    }
    if (!strcmp(a, "--seed")) {
      o->seed = value;
      continue;
    }
    if (!strcmp(a, "--time-lapse") && value >= 24 && value <= 3600) {
      o->time_lapse = (unsigned)value;
      continue;
    }
    if (!strcmp(a, "--decor-time") && value <= 1000000) {
      o->decor_time = (unsigned)value;
      o->fixed_decor = true;
      continue;
    }
    if (!strcmp(a, "--volume") && value <= 100) {
      o->volume = (unsigned)value;
      continue;
    }
    if (!strcmp(a, "--scene") && value >= 1 && value < 80) {
      o->scene = (int)value;
      continue;
    }
    if (!strcmp(a, "--story-day") && value >= 1 && value <= 11) {
      o->story_day = (int)value;
      continue;
    }
    if (!strcmp(a, "--skip-steps") && value <= 6000) {
      o->skip_steps = (unsigned)value;
      continue;
    }
    if (!strcmp(a, "--headless") && value <= 1000000) {
      o->headless = true;
      o->steps = value;
      continue;
    }
    fprintf(stderr, "Unknown option or invalid range: %s\n", a);
    return false;
  }
  if (o->walk_demo) {
    o->modern = true;
    if (!o->fixed_clock) {
      o->clock.hour = 12;
      o->clock.minute = 0;
      o->fixed_clock = true;
    }
  }
  if (o->skip_steps && (o->scene < 0 || o->walk_demo || o->list)) {
    fputs("--skip-steps requires --scene N\n", stderr);
    return false;
  }
  if (o->johnny_hd && !o->modern) {
    fputs("--johnny-hd requires --modern\n", stderr);
    return false;
  }
  if (o->walk_demo && (o->scene >= 0 || o->list)) {
    fputs("--walk-demo cannot force a story scene\n", stderr);
    return false;
  }
  if ((o->snapshot || o->verify) && !o->headless) {
    fputs("--snapshot/--verify require --headless N\n", stderr);
    return false;
  }
  if (o->audio_test && (o->headless || o->list)) {
    fputs("--audio-test cannot be combined with --headless/--list-scenes\n",
          stderr);
    return false;
  }
  return true;
}
static int audio_test(void) {
  const size_t count = JC_AUDIO_SOURCE_RATE * 4u;
  float *samples = calloc(count, sizeof *samples);
  if (!samples)
    return 1;
  /* Three separated, gently faded tones; no original assets or VM required. */
  for (size_t i = 0; i < count; ++i) {
    double t = (double)i / JC_AUDIO_SOURCE_RATE;
    int pulse = (int)t;
    double phase = t - pulse;
    if (pulse >= 1 && pulse <= 3 && phase < 0.65) {
      double envelope = fmin(1.0, fmin(phase, 0.65 - phase) / 0.02);
      samples[i] = (float)(0.16 * envelope *
          sin(6.283185307179586 * (220.0 + 220.0 * pulse) * phase));
    }
  }
  JCSound tone = {UINT32_MAX, samples, count};
  JCExe exe = {.sounds = &tone, .sound_count = 1};
  fprintf(stderr, "Audio test: three tones at 1, 2 and 3 seconds; "
                  "volume %u%%%s\n", app.options.volume,
          app.options.mute ? " (muted)" : "");
  JCAudio *audio = jc_audio_create(&exe, app.options.volume, app.options.mute);
  const int result = audio ? 0 : 1;
  if (audio) {
    jc_audio_events(audio, &tone.id, 1, true);
    struct timespec remaining = {4, 200000000};
    while (nanosleep(&remaining, &remaining) && errno == EINTR) {}
    jc_audio_destroy(audio);
  }
  free(samples);
  return result;
}
static bool start_vm(void) {
  Options *o = &app.options;
  JCConfig config = jc_config_default();
  app.persist = !o->headless && !o->list && !o->smoke_frames && !o->walk_demo &&
                !o->fixed_clock && o->scene < 0 && !o->story_day &&
                !o->time_lapse && !o->freeze_scene && !o->fixed_decor;
  if (app.persist) {
    JCError err = {{0}};
    if (!jc_config_load(o->config_path, &config, &err)) {
      fprintf(stderr,
              "Config: %s (defaults for this run; existing file preserved)\n",
              err.message);
      app.persist = false;
    }
  } else {
    config.num_days = config.intro = o->story_day ? o->story_day : 1;
    config.year = o->clock.year;
    config.month = o->clock.month;
    config.day = o->clock.day;
  }
  if (!vm_init(&app.vm, o->data_dir, o->seed, &o->clock, &config, o->scene)) {
    fprintf(stderr, "VM: %s\n", app.vm.error.message);
    return false;
  }
  if (o->walk_demo)
    jc_engine_walk_preview(app.vm.engine);
  for (unsigned i = 0; i < o->skip_steps; ++i)
    if (!vm_step(&app.vm)) {
      fprintf(stderr, "Scene advance: %s\n", app.vm.error.message);
      return false;
    }
  return true;
}
static int headless(void) {
  int result = 1;
  uint8_t *scratch = NULL;
  if (!start_vm())
    goto done;
  if (app.options.list) {
    for (size_t i = 1; i < app.vm.assets.exe.scene_count; ++i) {
      const JCSceneDef *s = &app.vm.assets.exe.scenes[i];
      printf("%2zu  ADS=%02x tag=%u day=%u phase=%u..%u flags=%04x start=%u/%u "
             "end=%u/%u\n",
             i, s->ads_id, s->ads_tag, s->day, s->min_phase, s->max_phase,
             (unsigned)s->flags, s->spot_start, s->hdg_start, s->spot_end,
             s->hdg_end);
    }
    result = 0;
    goto done;
  }
  if (app.options.verify) {
    scratch = malloc(WIDTH * HEIGHT);
    if (!scratch)
      goto done;
  }
  uint64_t hash = UINT64_C(14695981039346656037);
  for (uint64_t i = 0; i < app.options.steps; ++i) {
    if (!vm_step(&app.vm) || (scratch && !vm_verify_frame(&app.vm, scratch))) {
      fprintf(stderr, "VM: %s\n", app.vm.error.message);
      goto done;
    }
    const uint8_t *p = jc_engine_frame(app.vm.engine)->px;
    for (size_t k = 0; k < WIDTH * HEIGHT; ++k) {
      hash ^= p[k];
      hash *= UINT64_C(1099511628211);
    }
  }
  if (app.options.snapshot) {
    FILE *f = fopen(app.options.snapshot, "wb");
    if (!f) {
      perror(app.options.snapshot);
      goto done;
    }
    bool ok = fprintf(f, "P6\n%d %d\n255\n", WIDTH, HEIGHT) > 0;
    const uint8_t (*pal)[3] = jc_engine_palette(app.vm.engine);
    const uint8_t *p = jc_engine_frame(app.vm.engine)->px;
    for (size_t i = 0; i < WIDTH * HEIGHT && ok; ++i)
      ok = fwrite(pal[p[i]], 1, 3, f) == 3;
    if (fclose(f) != 0)
      ok = false;
    if (!ok) {
      fputs("Snapshot write failed\n", stderr);
      goto done;
    }
  }
  printf("headless: %llu steps, %u ticks, %llu scenes done, stream "
         "FNV64=%016llx, atlas fallbacks=%llu%s\n",
         (unsigned long long)app.vm.steps, jc_engine_now(app.vm.engine),
         (unsigned long long)jc_engine_scenes_done(app.vm.engine),
         (unsigned long long)hash, (unsigned long long)app.vm.flattened_frames,
         scratch ? ", capture pixel-exact" : "");
  result = 0;
done:
  free(scratch);
  vm_shutdown(&app.vm);
  return result;
}
static void sound_events(void *user, const uint32_t *ids, size_t count,
                         bool enabled) {
  jc_audio_events(user, ids, count, enabled);
}
static void init(void) {
  stm_setup();
  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
      .buffer_pool_size = 16,
      .image_pool_size = 16,
      .sampler_pool_size = 4,
      .shader_pool_size = 8,
      .pipeline_pool_size = 8,
      .view_pool_size = 32,
      .vulkan.copy_staging_buffer_size = JC_COPY_MIB * 1024 * 1024,
      .vulkan.transient_staging_buffer_size = JC_STREAM_MIB * 1024 * 1024});
  if (!sg_isvalid() || !start_vm())
    goto fail;
  const sg_desc accepted = sg_query_desc();
  VkPhysicalDeviceProperties device_info;
  vkGetPhysicalDeviceProperties(
      (VkPhysicalDevice)sapp_get_environment().vulkan.physical_device,
      &device_info);
  fprintf(stderr,
          "GPU: %s; staging accepted: %d copy bytes + %d stream bytes x %d "
          "frames\n",
          device_info.deviceName, accepted.vulkan.copy_staging_buffer_size,
          accepted.vulkan.transient_staging_buffer_size,
          SG_NUM_INFLIGHT_FRAMES);
  if (app.options.johnny_hd)
    fputs("--johnny-hd: legacy option; Johnny now always uses original R8 "
          "sprites\n",
          stderr);
  app.renderer =
      renderer_create(jc_engine_palette(app.vm.engine),
                      app.options.modern ? app.options.modern_path : NULL,
                      NULL);
  if (!app.renderer) {
    fputs("Renderer creation failed\n", stderr);
    goto fail;
  }
  app.audio = jc_audio_create(&app.vm.assets.exe, app.options.volume,
                              app.options.mute);
  app.vm.sound_observer = sound_events;
  app.vm.sound_user = app.audio;
  if (app.options.modern && !vm_step(&app.vm))
    goto fail;
  app.intro_left = app.options.smoke_frames || app.options.modern ? 0.0 : 2.0;
  app.last_time = stm_now();
  app.ready = true;
  return;
fail:
  app.exit_code = 1;
  sapp_request_quit();
}
static void frame(void) {
  if (stop_requested) {
    sapp_request_quit();
    return;
  }
  if (!app.ready)
    return;
  uint64_t now = stm_now();
  double elapsed = stm_sec(stm_diff(now, app.last_time));
  app.last_time = now;
  app.visual_seconds += elapsed;
  if (app.intro_left > 0) {
    app.intro_left -= elapsed;
    elapsed = 0;
  }
  const JCClock clock =
      app.options.fixed_clock ? app.options.clock : wall_clock();
  if (!vm_advance(&app.vm, app.options.freeze_scene ? 0.0 : elapsed, &clock)) {
    fprintf(stderr, "VM: %s\n", app.vm.error.message);
    app.exit_code = 1;
    app.ready = false;
    sapp_request_quit();
    return;
  }
  struct timespec lighting_now = {0};
  if (!app.options.fixed_clock)
    clock_gettime(CLOCK_REALTIME, &lighting_now);
  double decor_seconds = app.options.fixed_decor ? app.options.decor_time :
                            app.visual_seconds;
  double hour = clock.hour + clock.minute / 60.0 +
                                      (app.options.fixed_clock
                                           ? 0.0
                                           : ((lighting_now.tv_sec % 60) +
                                              lighting_now.tv_nsec / 1e9) /
                                                 3600.0);
  if (app.options.time_lapse)
    hour = fmod(hour + decor_seconds * 24.0 / app.options.time_lapse, 24.0);
  renderer_set_hour(app.renderer, hour);
  renderer_set_environment(app.renderer, decor_seconds,
                           !app.options.no_decor_fx);
  bool submitted =
      renderer_frame(app.renderer, &app.vm.scene, sglue_swapchain(),
                     !app.options.no_interpolation);
  sg_commit();
  if (submitted)
    ++app.frames;
  if (app.options.smoke_frames && app.frames >= app.options.smoke_frames) {
    fprintf(stderr,
            "Vulkan: %llu submitted frames, %llu VM steps, %zu final draw "
            "commands\n",
            (unsigned long long)app.frames, (unsigned long long)app.vm.steps,
            app.vm.scene.current->count);
    sapp_request_quit();
  }
}
static void cleanup(void) {
  jc_audio_destroy(app.audio);
  app.audio = NULL;
  if (app.ready && app.persist && app.vm.engine) {
    JCError err = {{0}};
    if (!jc_config_save(app.options.config_path,
                        jc_engine_config(app.vm.engine), &err))
      fprintf(stderr, "Config: %s\n", err.message);
  }
  if (sg_isvalid()) {
    renderer_destroy(app.renderer);
    sg_shutdown();
  }
  vm_shutdown(&app.vm);
}
static void event(const sapp_event *ev) {
  if (ev->type == SAPP_EVENTTYPE_KEY_DOWN && !ev->key_repeat &&
      ev->key_code == SAPP_KEYCODE_M)
    jc_audio_toggle_mute(app.audio);
  if (ev->type == SAPP_EVENTTYPE_KEY_DOWN &&
      ev->key_code == SAPP_KEYCODE_ESCAPE)
    sapp_request_quit();
}
int main(int argc, char **argv) {
  if (!parse_options(argc, argv, &app.options))
    return 2;
  if (app.options.audio_test)
    return audio_test();
  if (app.options.headless || app.options.list)
    return headless();
  struct sigaction action = {0};
  action.sa_handler = stop_signal;
  sigemptyset(&action.sa_mask);
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGINT, &action, NULL);
  const sapp_desc desc = {.init_cb = init,
                          .frame_cb = frame,
                          .cleanup_cb = cleanup,
                          .event_cb = event,
                          .width = 1280,
                          .height = app.options.modern ? 720 : 960,
                          .window_title = "Johnny Castaway | C99 | Vulkan",
                          .sample_count = 1,
                          .depth_format = SAPP_PIXELFORMAT_NONE,
                          .high_dpi = true,
                          .fullscreen = app.options.fullscreen,
                          .disable_vsync = false,
                          .composite_mode = SAPP_COMPOSITEMODE_OPAQUE,
                          .logger.func = slog_func};
  sapp_run(&desc);
  return app.exit_code;
}
