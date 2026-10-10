// SPDX-License-Identifier: GPL-3.0-only
/* The interface: reads the controls once per app_frame, handles what every view shares (PLAY,
 * octave, edit step, the view buttons), hands the rest to the current view, lights the LEDs and
 * has the view redraw what changed (spec §3 and §4). Views: Phrase (SEQ) and Project (GLO, ADR-031);
 * the others come with P2. */
#pragma once
#include <stdint.h>
#include "app/plat.h"

enum { VIEW_PHRASE, VIEW_PROJECT, VIEW_COUNT };

#define UI_OCT_MIN 0
#define UI_OCT_MAX 8
#define UI_OCT_START 3                  /* key 0 (F3) plays F-3 */
#define UI_STEP_MAX 15                  /* edit step: rows the cursor moves down after a note */
#define UI_HEX_TIMEOUT_MS 1500          /* [PROTO] a lone hex digit is taken after this (Q-04) */

typedef struct {
    uint8_t view;
    uint8_t track;                      /* 0..7, shown 1..8 */
    uint8_t phrase;                     /* the phrase the Phrase view edits */
    uint8_t row, col;                   /* the cursor: step 0..F, column STEP_* */
    uint8_t oct;                        /* keyboard octave */
    uint8_t step;                       /* edit step, 0..F */
    uint8_t instr;                      /* the instrument the keys play and new notes get */
    int8_t pending;                     /* the first hex digit typed, shown "7_"; -1 none */
} ui_t;

extern ui_t ui;

/* the controls of one frame */
typedef struct {
    uint32_t held, pressed;             /* buttons: bit B_* */
    uint32_t keys, keys_down;           /* keys: bit k */
    int enc[EN_COUNT];                  /* detents since the last frame */
    uint32_t now;                       /* plat_ms() */
} input_t;

#define HELD(in, b) ((in)->held >> (b) & 1u)
#define PRESSED(in, b) ((in)->pressed >> (b) & 1u)

void ui_init(void);                     /* after model_init() and seq_init() */
void ui_frame(void);                    /* from app_frame */

/* keys: the white index of a key (0..15), -1 for a black one; the bit of white key w */
int ui_white(int key);
uint32_t ui_white_bit(int w);

/* text helpers for the views: always NUL-terminated */
void ui_hex2(int v, char *s);           /* "7F" */
void ui_note(int note, char *s);        /* "C-4", "C#4", "OFF", "---" (empty), "???" */

/* app state for the scenario runner's expect (host/scenario.c) */
enum { UI_PROBE_VIEW, UI_PROBE_TRACK, UI_PROBE_PHRASE, UI_PROBE_ROW, UI_PROBE_COL, UI_PROBE_OCT,
       UI_PROBE_STEP, UI_PROBE_INSTR, UI_PROBE_PENDING, UI_PROBE_FIELD, UI_PROBE_CHAR };
int ui_probe(int what);
