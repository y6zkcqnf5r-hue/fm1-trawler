// SPDX-License-Identifier: GPL-3.0-only
/* The Project view (spec §3, minimal for P1-07, ADR-031): the tempo and the project's name.
 *
 *   SELECT     the field: TEMPO, NAME
 *   PRESETS    TEMPO +-1 BPM (SEL: +-10), 40..300; NAME: the character under the cursor goes
 *              through " A..Z 0..9 - _ . #" (SEL: 10 at a time)
 *   ALGORITHM  on NAME, the character position (0..11)
 *   SEL+EDIT   on NAME, clears the character
 * Slots, new, load and save come with P2-08. */
#pragma once
#include <stdint.h>
#include "ui/ui.h"

void view_project_input(const input_t *in);
void view_project_draw(int full);
int view_project_probe(int what);       /* PROJ_PROBE_* */
enum { PROJ_PROBE_FIELD, PROJ_PROBE_CHAR };
