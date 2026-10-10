// SPDX-License-Identifier: GPL-3.0-only
/* One voice per track (spec §2), placeholder until the WAV engine (P1-03): a sine with a 2 ms attack
 * and a ~90 ms release. A new note on a sounding voice first fades it to 0 in 96 frames (6
 * sub-blocks, 2.2 ms), then starts at phase 0, so it never clicks; on a silent voice it starts on
 * the very sample it is called at.
 *
 * The envelope moves once per sub-block of 16 frames (CLAUDE.md rule 5), the gain is interpolated
 * across it. The sub-block counter belongs to the voice and restarts with each note, so the output
 * does not depend on how the caller cuts its blocks. */
#pragma once
#include <stdint.h>
#include "dsp/fixed.h"

typedef struct {
    phase_t ph;
    uint32_t inc;
    int32_t env;                        /* Q15 */
    int32_t level;                      /* Q15, the note's VOL */
    int32_t g0, g1;                     /* Q15 gain at the start and the end of the sub-block */
    uint32_t next_inc;                  /* the note waiting for the fade */
    int32_t next_level;
    uint8_t state, sub;
} voice_t;

void voice_reset(voice_t *v);           /* silent at once */
void voice_note(voice_t *v, int note, int vol);   /* note 0..119, vol 0..FF */
void voice_legato(voice_t *v, int note);          /* new pitch, no new attack; triggers if silent */
void voice_vol(voice_t *v, int vol);
void voice_off(voice_t *v);             /* release */
int voice_active(const voice_t *v);     /* 0 = silent, adds nothing */
void voice_render(voice_t *v, int32_t *lr, int n);  /* adds n interleaved L R frames, Q15 */
