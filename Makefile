# SPDX-License-Identifier: GPL-3.0-only
# Trawler build. Works with GNU Make 3.81 (the macOS one).
#
#   make host    native simulator: build/host/trawler           (P0-03)
#   make web     wasm build: build/emu/                         (P0-05; DOCKER=1 uses the pinned image)
#   make serve   serve build/emu on http://localhost:8080       (P0-05)
#   make test    lint + lint self-test + unit tests
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

.PHONY: all host web serve test lint lint-selftest clean

all: test

host:
	@echo "make host: arrive en P0-03 (simulateur hôte)"; exit 1

web:
	@echo "make web: arrive en P0-05 (build wasm, emcc = $(EMCC))"; exit 1

serve:
	@echo "make serve: arrive en P0-05"; exit 1

lint:
	@sh tools/lint.sh

lint-selftest:
	@sh tests/lint/selftest.sh

test: lint lint-selftest

clean:
	rm -rf build
