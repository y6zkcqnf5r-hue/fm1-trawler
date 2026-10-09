# SPDX-License-Identifier: GPL-3.0-only
# Trawler build. Works with GNU Make 3.81 (the macOS one).
#
#   make host    native simulator: build/host/trawler SCRIPT.trs [OUTDIR] -> WAV + PNG
#   make web     wasm build: build/emu/{index.html, worklet.js, keymap.js, panel.js, trawler.wasm}
#                (emcc from the local emsdk at EMSDK_VERSION; DOCKER=1 uses the pinned image)
#   make serve   make web, then serve build/emu on http://localhost:8080
#   make test    lint + lint self-test + generated tables + unit tests + scenarios + wasm/native parity
#   make parity  tests/host/emu_test.mjs: the wasm and the native build give the same bytes
#                (PARITY=0 skips it, loudly: CLAUDE.md rule 7 is then not checked)
#   make scenarios  every tests/scenarios/*.trs into build/scenarios/<name>/, outputs checked
#                against tests/golden/<name>.sha256
#   make golden  play the scenarios and accept their WAV and PNG as the new golden hashes
#   make gen     build/gen/tables.h (tools/gen_tables.py), build/gen/font.h (tools/gen_font.py)
#   make lint    chip rules on firmware/src, SPDX headers everywhere
#   make clean

CC ?= cc
CFLAGS_COMMON = -std=gnu99 -O2 -Wall -Wextra -Werror=double-promotion -ffp-contract=off -fwrapv \
                -Ifirmware/src -Ibuild/gen

# Emscripten, pinned (ADR-011): local emsdk at the same version, or the Docker image
EMSDK_VERSION := 6.0.11
EMSDK_DIR ?= $(HOME)/emsdk
EMCC ?= $(shell command -v emcc 2>/dev/null || echo $(EMSDK_DIR)/upstream/emscripten/emcc)
EMSDK_PYTHON ?= $(firstword $(wildcard /opt/homebrew/bin/python3.12 /usr/local/bin/python3.12))
ifneq ($(EMSDK_PYTHON),)
export EMSDK_PYTHON
endif
ifeq ($(DOCKER),1)
EMCC = docker run --rm -v "$(CURDIR)":/src -w /src emscripten/emsdk:$(EMSDK_VERSION) emcc
endif

PYTHON ?= python3
GEN = build/gen/tables.h build/gen/font.h
FONT_BDF = assets/fonts/ter-u12n.bdf
UNIT_SRC = $(wildcard tests/unit/test_*.c)
UNIT_BIN = $(patsubst tests/unit/%.c,build/unit/%,$(UNIT_SRC))
FIRMWARE_H = $(wildcard firmware/src/*/*.h)
APP_SRC = $(filter-out %/plat_fm1.c,$(wildcard firmware/src/*/*.c))
HOST_SRC = host/trawler_host.c host/scenario.c host/wav.c host/png.c
HOST_BIN = build/host/trawler
SCENARIOS = $(wildcard tests/scenarios/*.trs)
WEB_SRC = web/emu/trawler_web.c host/trawler_host.c
WEB_PAGE = build/emu/index.html build/emu/worklet.js build/emu/keymap.js build/emu/panel.js
WASM = build/emu/trawler.wasm
NODE ?= node

.PHONY: all host web serve gen test unit scenarios run-scenarios golden parity lint lint-selftest clean

all: test

host: $(HOST_BIN)

$(HOST_BIN): $(HOST_SRC) $(APP_SRC) $(wildcard host/*.h) $(FIRMWARE_H) $(GEN)
	@mkdir -p build/host
	$(CC) $(CFLAGS_COMMON) -DAPP_HOST -Ihost -o $@ $(HOST_SRC) $(APP_SRC)

run-scenarios: $(HOST_BIN)
	@rm -rf build/scenarios
	@for s in $(SCENARIOS); do \
	    n=$$(basename $$s .trs); mkdir -p build/scenarios/$$n; \
	    echo "scenario $$n"; $(HOST_BIN) $$s build/scenarios/$$n || exit 1; \
	done
	@$(PYTHON) tests/host/check_outputs.py build/scenarios

scenarios: run-scenarios
	@$(PYTHON) tests/host/golden.py check build/scenarios tests/golden

golden: run-scenarios
	@$(PYTHON) tests/host/golden.py accept build/scenarios tests/golden

web: $(WASM) $(WEB_PAGE)

# same flags as the native build (rule 7), no Emscripten glue: the worklet brings a stub WASI
$(WASM): $(WEB_SRC) $(APP_SRC) $(wildcard host/*.h) $(FIRMWARE_H) $(GEN)
	@mkdir -p build/emu
	@if [ "$(DOCKER)" != 1 ] && ! command -v $(firstword $(EMCC)) >/dev/null 2>&1; then \
	    echo "make web: emcc introuvable ($(EMCC))."; \
	    echo "  Installer emsdk $(EMSDK_VERSION) dans $(EMSDK_DIR), ou lancer avec DOCKER=1 (image emscripten/emsdk:$(EMSDK_VERSION))."; \
	    echo "  Pour sauter la parité dans make test : make test PARITY=0"; exit 1; fi
	$(EMCC) $(CFLAGS_COMMON) -DAPP_HOST -DAPP_WEB -Ihost --no-entry -sSTANDALONE_WASM -sFILESYSTEM=0 \
	    -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=33554432 -o $@ $(WEB_SRC) $(APP_SRC)
	@echo "web: $@ ($$(wc -c < $@ | tr -d ' ') octets)"

build/emu/%: web/emu/%
	@mkdir -p build/emu
	cp $< $@

serve: web
	@echo "Trawler : http://localhost:8080 (Ctrl+C pour arrêter)"
	@$(PYTHON) -m http.server 8080 --bind 127.0.0.1 --directory build/emu

ifeq ($(PARITY),0)
parity:
	@echo "SKIP parité (PARITY=0) : règle 7 NON vérifiée, le wasm et le natif ne sont pas comparés"
else
parity: $(HOST_BIN) $(WASM) web/emu/worklet.js web/emu/keymap.js web/emu/panel.js
	@$(NODE) tests/host/emu_test.mjs
endif

lint:
	@sh tools/lint.sh

lint-selftest:
	@sh tests/lint/selftest.sh

gen: $(GEN)

build/gen/tables.h: tools/gen_tables.py
	@$(PYTHON) tools/gen_tables.py $@

build/gen/font.h: tools/gen_font.py $(FONT_BDF)
	@$(PYTHON) tools/gen_font.py $(FONT_BDF) $@

build/unit/%: tests/unit/%.c tests/unit/test.h $(FIRMWARE_H) $(GEN)
	@mkdir -p build/unit
	$(CC) $(CFLAGS_COMMON) -Itests/unit -o $@ $< -lm

unit: $(UNIT_BIN)
	@for t in $(UNIT_BIN); do $$t || exit 1; done

test: lint lint-selftest unit scenarios parity

clean:
	rm -rf build
