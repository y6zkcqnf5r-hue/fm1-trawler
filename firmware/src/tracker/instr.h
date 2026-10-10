// SPDX-License-Identifier: GPL-3.0-only
/* Instrument types and the layout of instr_t.p[] (spec §5, ADR-025). The common parameters come
 * first, the same for every type, then the engine's own from IP_ENGINE. Saved projects store these
 * offsets: new parameters go at the end of their group's free bytes, never in between. */
#pragma once
#include "dsp/voice.h"

enum { INSTR_WAV, INSTR_FM4, INSTR_DRM, INSTR_TYPES };   /* INSTR_NONE (FF): no engine */

enum {
    IP_TRN, IP_FIN, IP_VOL, IP_PAN,     /* signed semitones, signed 1/128 semitone, v/0x80, 80 centre */
    IP_FLT, IP_CUT, IP_RES, IP_DRV,     /* FLT_*: OFF LP HP BP */
    IP_ATK, IP_DEC, IP_SUS, IP_REL,
    IP_E2ATK, IP_E2DEC, IP_E2DST, IP_E2AMT,
    IP_LFOSHP, IP_LFORAT, IP_LFODST, IP_LFOAMT,
    IP_SCH, IP_SDL, IP_SRV,
    IP_ENGINE = 24,                     /* the engine's own parameters */
    IP_WAV_SHP = IP_ENGINE, IP_WAV_PW, IP_WAV_VOX, IP_WAV_DET, IP_WAV_SUB,
};

/* set the type and every parameter to that type's defaults (through instr_set: undo sees it);
 * INSTR_NONE clears them. 0, or -1 if refused. */
int instr_set_type(int i, int type);

/* what a note of instrument i needs (engine VOICE_NONE if the instrument has no engine yet) */
void instr_patch(int i, patch_t *p);
int instr_pitch(int i, int note);       /* note + TRN, in 1/16 semitone with FIN */
