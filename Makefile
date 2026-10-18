CC ?= cc
CPPFLAGS += -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -DSOKOL_VULKAN -isystem vendor
CFLAGS ?= -std=c99 -Os -flto -DNDEBUG -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -Wall -Wextra -Wpedantic -Werror
LDFLAGS += -flto -Wl,--gc-sections -Wl,-s -Wl,--build-id=none
LDLIBS += -lX11 -lXi -lXcursor -lvulkan -lasound -ldl -lm -pthread

CORE = resource.c rng.c gfx.c ttm.c ads.c island.c walk.c engine.c config.c
SOURCES = main.c vm_core.c render_frame.c renderer_vulkan.c environment.c audio.c audio_mixer.c $(CORE)
HEADERS = $(wildcard *.h) $(wildcard generated/*.h) $(wildcard vendor/*.h)
TESTDIR ?= work/tests
TESTFLAGS = -std=c99 -O1 -g -I. -Wall -Wextra -Wpedantic -Werror $(SANITIZE)

.PHONY: all clean test size sanitize shaders compact
all: johnny-hd2d

johnny-hd2d: Makefile $(SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -pthread $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

test:
	mkdir -p $(TESTDIR)
	$(CC) $(TESTFLAGS) environment.c tests/test_environment.c -lm -o $(TESTDIR)/environment
	$(TESTDIR)/environment
	$(CC) $(TESTFLAGS) audio_mixer.c $(CORE) vm_core.c render_frame.c tests/test_audio.c -lm -o $(TESTDIR)/audio
	$(TESTDIR)/audio assets/original
	$(CC) $(TESTFLAGS) resource.c tests/test_resource.c -o $(TESTDIR)/resource
	$(TESTDIR)/resource assets/original
	$(CC) $(TESTFLAGS) $(CORE) tests/test_vm.c -lm -o $(TESTDIR)/vm
	$(TESTDIR)/vm assets/original
	$(CC) $(TESTFLAGS) $(CORE) tests/test_engine.c -lm -o $(TESTDIR)/engine
	$(TESTDIR)/engine assets/original
	$(CC) $(TESTFLAGS) render_frame.c gfx.c tests/test_scene.c -lm -o $(TESTDIR)/capture
	$(TESTDIR)/capture
	$(CC) $(CPPFLAGS) $(TESTFLAGS) $(CORE) tests/test_config.c -lm -o $(TESTDIR)/config
	$(TESTDIR)/config $(TESTDIR)
	$(CC) $(TESTFLAGS) $(CORE) vm_core.c render_frame.c tests/test_capture.c -lm -o $(TESTDIR)/capture-scenes
	$(TESTDIR)/capture-scenes assets/original
	$(CC) $(TESTFLAGS) $(CORE) vm_core.c render_frame.c tests/test_walk_hd.c -lm -o $(TESTDIR)/walk-hd
	$(TESTDIR)/walk-hd assets/original
	$(CC) $(TESTFLAGS) tests/test_layout.c -lm -o $(TESTDIR)/layout
	$(TESTDIR)/layout
	$(CC) $(TESTFLAGS) $(CORE) vm_core.c render_frame.c tests/test_character.c -lm -o $(TESTDIR)/characters
	$(TESTDIR)/characters assets/original
	$(CC) $(TESTFLAGS) $(CORE) vm_core.c render_frame.c tests/test_reading.c -lm -o $(TESTDIR)/reading
	$(TESTDIR)/reading assets/original
	$(CC) $(TESTFLAGS) $(CORE) vm_core.c render_frame.c tests/test_fishing.c -lm -o $(TESTDIR)/fishing
	$(TESTDIR)/fishing assets/original
	$(CC) $(TESTFLAGS) $(CORE) vm_core.c render_frame.c tests/test_hybrid.c -lm -o $(TESTDIR)/hybrid
	$(TESTDIR)/hybrid assets/original

sanitize:
	ASAN_OPTIONS=detect_leaks=0 $(MAKE) test SANITIZE='-fsanitize=address,undefined -fno-omit-frame-pointer'

.PHONY: inventory
inventory:
	mkdir -p $(TESTDIR) assets/scenes
	$(CC) $(TESTFLAGS) $(CORE) vm_core.c render_frame.c tools/scene_inventory.c -lm -o $(TESTDIR)/inventory
	$(TESTDIR)/inventory assets/original > assets/scenes/inventory.json.tmp
	mv assets/scenes/inventory.json.tmp assets/scenes/inventory.json

SHDC ?= sokol-shdc
shaders:
	$(SHDC) --input shaders/palette.glsl --output generated/palette.glsl.h --slang spirv_vk --no-log-cmdline

	$(SHDC) --input shaders/modern.glsl --output generated/modern.glsl.h --slang spirv_vk --no-log-cmdline
	$(SHDC) --input shaders/character.glsl --output generated/character.glsl.h --slang spirv_vk --no-log-cmdline
	$(SHDC) --input shaders/foliage.glsl --output generated/foliage.glsl.h --slang spirv_vk --no-log-cmdline

UPX_BIN ?= upx
compact: johnny-hd2d-compact

johnny-hd2d-compact: johnny-hd2d
	rm -f $@
	$(UPX_BIN) --best --lzma -o $@ johnny-hd2d

size: johnny-hd2d
	wc -c johnny-hd2d
	size johnny-hd2d

clean:
	rm -f johnny-hd2d johnny-hd2d-compact
