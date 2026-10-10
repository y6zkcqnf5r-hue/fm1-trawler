// SPDX-License-Identifier: GPL-3.0-only
/* The interface: reads the controls once per app_frame, moves the cursor, plays the keys, lights
 * the LEDs and has the current view redraw what changed (spec §3 and §4). Only the Phrase view
 * exists yet (P1-05); the others come with P2. */
#pragma once
#include <stdint.h>

enum { VIEW_PHRASE, VIEW_COUNT };

#define UI_OCT_MIN 0
#define UI_OCT_MAX 8
#define UI_OCT_START 3                  /* key 0 (F3) plays F-3 */

typedef struct {
    uint8_t view;
    uint8_t track;                      /* 0..7, shown 1..8 */
    uint8_t phrase;                     /* the phrase the Phrase view edits */
    uint8_t row, col;                   /* the cursor: step 0..F, column STEP_* */
    uint8_t oct;                        /* keyboard octave */
    uint8_t instr;                      /* the instrument the keys play */
} ui_t;

extern ui_t ui;

void ui_init(void);                     /* after model_init() and seq_init() */
void ui_frame(void);                    /* from app_frame */

/* text helpers for the views: always NUL-terminated */
void ui_hex2(int v, char *s);           /* "7F" */
void ui_note(int note, char *s);        /* "C-4", "C#4", "OFF", "---" (empty), "???" */

/* app state for the scenario runner's expect (host/scenario.c) */
enum { UI_PROBE_VIEW, UI_PROBE_TRACK, UI_PROBE_PHRASE, UI_PROBE_ROW, UI_PROBE_COL, UI_PROBE_OCT };
int ui_probe(int what);
