// SPDX-License-Identifier: GPL-3.0-only
#include "tracker/instr.h"
#include "tracker/model.h"

#define P(k) (INSTR_P + (k))

/* WAV: a sawtooth through a low-pass, short decay to a high sustain */
static const uint8_t WAV_DEFAULTS[][2] = {
    {IP_VOL, 0x80}, {IP_PAN, 0x80}, {IP_FLT, FLT_LP}, {IP_CUT, 0xC0}, {IP_RES, 0x20},
    {IP_ATK, 0x00}, {IP_DEC, 0x80}, {IP_SUS, 0xC0}, {IP_REL, 0x70},
    {IP_WAV_SHP, SHP_SAW}, {IP_WAV_PW, 0x80}, {IP_WAV_VOX, 0x01},
};

int instr_set_type(int i, int type)
{
    int k;
    if (i < 0 || i >= TRK_INSTRS || (type != INSTR_NONE && (type < 0 || type >= INSTR_TYPES)))
        return -1;
    instr_set(i, INSTR_TYPE, type);
    for (k = 0; k < INSTR_BYTES - INSTR_P; k++)
        instr_set(i, P(k), 0);
    if (type == INSTR_WAV)
        for (k = 0; k < (int)(sizeof WAV_DEFAULTS / sizeof *WAV_DEFAULTS); k++)
            instr_set(i, P(WAV_DEFAULTS[k][0]), WAV_DEFAULTS[k][1]);
    return 0;
}

void instr_patch(int i, patch_t *p)
{
    const instr_t *in = &g_proj->instr[i < 0 || i >= TRK_INSTRS ? 0 : i];
    p->engine = i >= 0 && i < TRK_INSTRS && in->type == INSTR_WAV ? VOICE_WAV : VOICE_NONE;
    p->shape = in->p[IP_WAV_SHP] < SHP_COUNT ? in->p[IP_WAV_SHP] : SHP_SAW;
    p->flt = in->p[IP_FLT] < FLT_COUNT ? in->p[IP_FLT] : FLT_OFF;
    p->cut = in->p[IP_CUT];
    p->res = in->p[IP_RES];
    p->atk = in->p[IP_ATK];
    p->dec = in->p[IP_DEC];
    p->sus = in->p[IP_SUS];
    p->rel = in->p[IP_REL];
    p->vol = in->p[IP_VOL];
    p->pan = in->p[IP_PAN];
}

int instr_pitch(int i, int note)
{
    const instr_t *in = &g_proj->instr[i < 0 || i >= TRK_INSTRS ? 0 : i];
    int n = note + (int8_t)in->p[IP_TRN];
    if (n < 0)
        n = 0;
    if (n > NOTE_MAX)
        n = NOTE_MAX;
    return n * 16 + ((int8_t)in->p[IP_FIN] >> 3);   /* FIN: +-1 semitone in 1/16 steps */
}
