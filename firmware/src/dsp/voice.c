// SPDX-License-Identifier: GPL-3.0-only
#include "dsp/voice.h"

#define SUB 16
#define SUB_SHIFT 4
#define ATK_STEP 5462                   /* 0 -> full in 6 sub-blocks, 2.2 ms */
#define FADE_STEP 5462                  /* full -> 0 in 6 sub-blocks: the 2 ms fade of a new note */
#define REL_STEP 128                    /* full -> 0 in 256 sub-blocks, 93 ms */

enum { V_IDLE, V_ON, V_REL, V_FADE };

static int32_t vol_q15(int vol) { return vol << 7 | vol >> 1; }   /* FF -> 32767 */

void voice_reset(voice_t *v)
{
    v->ph = 0;
    v->inc = v->next_inc = 0;
    v->env = v->g0 = v->g1 = 0;
    v->level = v->next_level = 0;
    v->state = V_IDLE;
    v->sub = 0;
}

static void start(voice_t *v, uint32_t inc, int32_t level)
{
    v->ph = 0;                          /* the sine starts at 0: no step */
    v->inc = inc;
    v->level = level;
    v->env = 0;
    v->state = V_ON;
    v->sub = 0;                         /* the attack starts on this sample */
}

/* end the sub-block here, at the gain reached so far: the fade starts on this sample */
static void cut_sub(voice_t *v)
{
    if (v->sub) {
        v->g1 = v->g0 + (((v->g1 - v->g0) * v->sub) >> SUB_SHIFT);
        v->sub = 0;
    }
}

void voice_note(voice_t *v, int note, int vol)
{
    uint32_t inc = note_inc(note, 0);
    if (!voice_active(v))
        start(v, inc, vol_q15(vol));
    else {
        cut_sub(v);
        v->next_inc = inc;
        v->next_level = vol_q15(vol);
        v->state = V_FADE;
    }
}

void voice_legato(voice_t *v, int note)
{
    if (v->state == V_ON)
        v->inc = note_inc(note, 0);
    else if (v->state == V_FADE)
        v->next_inc = note_inc(note, 0);
    else
        voice_note(v, note, 0xFF);
}

void voice_vol(voice_t *v, int vol)
{
    if (v->state == V_FADE)
        v->next_level = vol_q15(vol);
    else
        v->level = vol_q15(vol);
}

void voice_off(voice_t *v)
{
    if (v->state == V_ON)
        v->state = V_REL;
    else if (v->state == V_FADE)        /* the waiting note will not start */
        v->state = V_REL;
}

/* silent once released and the last sub-block has ramped down to 0 */
int voice_active(const voice_t *v) { return v->state != V_IDLE || v->g1 != 0 || v->sub != 0; }

/* the envelope of the next sub-block */
static void next_sub(voice_t *v)
{
    v->g0 = v->g1;
    if (v->state == V_FADE && v->g0 == 0)
        start(v, v->next_inc, v->next_level);
    switch (v->state) {
    case V_ON:
        v->env = v->env + ATK_STEP < Q15_ONE ? v->env + ATK_STEP : Q15_ONE;
        break;
    case V_REL:
        v->env = v->env > REL_STEP ? v->env - REL_STEP : 0;
        if (!v->env)
            v->state = V_IDLE;
        break;
    case V_FADE:
        v->env = v->env > FADE_STEP ? v->env - FADE_STEP : 0;
        break;
    default:
        v->env = 0;
    }
    v->g1 = (v->env * v->level) >> 15;
}

void voice_render(voice_t *v, int32_t *lr, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        int32_t g, s;
        if (v->sub == 0) {
            if (!voice_active(v))
                return;                 /* silent: adds nothing, and nothing to keep in step */
            next_sub(v);
        }
        g = v->g0 + (((v->g1 - v->g0) * (v->sub + 1)) >> SUB_SHIFT);
        s = ((int32_t)sin_q15(v->ph) * g) >> 16;    /* -6 dB: a full sine is +-16383 */
        v->ph += v->inc;
        lr[2 * i] += s;
        lr[2 * i + 1] += s;
        v->sub = (uint8_t)((v->sub + 1) & (SUB - 1));
    }
}
