// SPDX-License-Identifier: GPL-3.0-only
/* The Phrase view (spec §3, §4): drawing (ui/view_phrase.c) and editing (ui/phrase_edit.c).
 *
 * Drawing: header, 16 steps x NOTE VOL INSTR FX1..FX3, the four knob boxes and a help line. It
 * keeps what it drew and redraws only the rows, the header and the footer whose content changed.
 *
 * Editing, by column under the cursor (ADR-029):
 *   NOTE       the 27 keys play the note, write it (and the instrument, if the step has none),
 *              the cursor moves down by the edit step
 *   VOL INSTR  the 16 white keys are hex digits: the first shows "7_", the second completes the
 *   FX values  value; a lone digit is taken after UI_HEX_TIMEOUT_MS, or at the next action
 *   FX         white keys 1..6 pick a family (pitch, time, structure, sound, sends, global);
 *              again on the same key, the next command of that family
 *   PRESETS    the value +-1 (a note +-1 semitone, a command the next one); with SEL +-16
 *              (a note +-1 octave, a command the next family); on an empty cell, the last value
 *   EDIT       on an empty cell, the last value; on a note, OFF (on OFF, the last note); on another
 *              value, it becomes the last value. SEL+EDIT clears the cell
 *   KNOB1..4   CUT RES SDL SRV of the instrument, live; with SEL +-16 */
#pragma once
#include <stdint.h>
#include "ui/ui.h"

void view_phrase_init(void);            /* the last values, at boot */
void view_phrase_input(const input_t *in);
uint32_t view_phrase_dim(void);         /* the keys to light dim: what they do in this column */
void view_phrase_draw(int full);        /* full: the whole screen (on entering the view) */
