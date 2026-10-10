// SPDX-License-Identifier: GPL-3.0-only
/* The Phrase view (spec §3): header, 16 steps x NOTE VOL INSTR FX1..FX3, the four knob boxes and a
 * help line. It keeps what it drew and redraws only the rows, the header and the footer whose
 * content changed. */
#pragma once

void view_phrase_draw(int full);        /* full: the whole screen (on entering the view) */
