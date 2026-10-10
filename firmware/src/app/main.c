// SPDX-License-Identifier: GPL-3.0-only
/* Trawler: the app the platform runs (plat.h). The tracker's interface (ui/), its sequencer and
 * voices (tracker/, dsp/), and MASTER as the overall volume.
 *
 * Until P1-06 lets you type a phrase, phrase 00 holds a demo (ADR-024): PLAY loops it. */
#include "app/plat.h"
#include "dsp/fixed.h"
#include "tracker/instr.h"
#include "tracker/model.h"
#include "tracker/seq.h"
#include "ui/ui.h"

static volatile int32_t v_master;       /* MASTER, Q12: written by app_frame, read by app_audio */
static int32_t master_g;                /* the gain reached at the end of the last block */

/* C-4 D#4 G-4 A#4 on steps 0 4 8 12 (G-4 at VOL 80), OFF on step E, instrument 00 (WAV) */
static void demo_phrase(void)
{
    static const uint8_t NOTES[4] = {48, 51, 55, 58};
    int i;
    for (i = 0; i < 4; i++) {
        phrase_set(0, 4 * i, STEP_NOTE, NOTES[i]);
        phrase_set(0, 4 * i, STEP_INSTR, 0);
    }
    phrase_set(0, 8, STEP_VOL, 0x80);
    phrase_set(0, 14, STEP_NOTE, NOTE_OFF);
    instr_set_type(0, INSTR_WAV);       /* a saw through a resonant low-pass */
    instr_set(0, INSTR_P + IP_CUT, 0x90);
    instr_set(0, INSTR_P + IP_RES, 0x80);
}

void app_init(void)
{
    model_init();
    demo_phrase();
    seq_init();
    ui_init();
    v_master = master_g = plat_master();
}

void app_frame(void)
{
    ui_frame();
    v_master = plat_master();
}

void app_audio(int32_t *lr, int n)
{
    int32_t m0 = master_g, m1 = v_master, g = m0;
    int i;
    seq_render(lr, n);
    for (i = 0; i < n; i++) {           /* MASTER ramps across the block (n = 256): no zipper */
        g = i >= 255 ? m1 : m0 + (((m1 - m0) * (i + 1)) >> 8);
        lr[2 * i] = (lr[2 * i] * g) >> 12;      /* 8 voices * 2^15 * 2^12 < 2^31 */
        lr[2 * i + 1] = (lr[2 * i + 1] * g) >> 12;
    }
    master_g = g;
}
