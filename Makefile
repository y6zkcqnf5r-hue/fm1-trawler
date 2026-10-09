# SPDX-License-Identifier: GPL-3.0-only
# Trawler build. Works with GNU Make 3.81 (the macOS one).
#
#   make host    native simulator: build/host/trawler SCRIPT.trs [OUTDIR] -> WAV + PNG
#   make web     wasm build: build/emu/                         (P0-05; DOCKER=1 uses the pinned image)
#   make serve   serve build/emu on http://localhost:8080       (P0-05)
#   make test    lint + lint self-test + generated tables + unit tests + scenarios
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

.PHONY: all host web serve gen test unit scenarios run-scenarios golden lint lint-selftest clean

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

web:
	@echo "make web: arrive en P0-05 (build wasm, emcc = $(EMCC))"; exit 1

serve:
	@echo "make serve: arrive en P0-05"; exit 1

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

test: lint lint-selftest unit scenarios

clean:
	rm -rf build
