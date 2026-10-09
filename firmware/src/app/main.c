// SPDX-License-Identifier: GPL-3.0-only
/* The "hello" app (P0-04): a monophonic sine on the 27 keys, last note wins, octave by OCT- and
 * OCT+, a short envelope against clicks, MASTER as the output gain. The screen shows the app's
 * name, the note and the octave; the held keys light. P1 replaces it with the tracker. */
#include "app/plat.h"
#include "dsp/fixed.h"
#include "ui/gfx.h"

#define C_BG 0x10A2u                    /* RGB565 */
#define C_TEXT 0xFFFFu
#define C_DIM 0x8410u
#define C_NOTE 0xFFE0u
#define C_OFF_WHITE 0x8410u
#define C_OFF_BLACK 0x2104u
#define C_KEY_ON 0xFFE0u

#define KEY_W 15                        /* 16 white keys across 240 px */
#define KEYS_Y 176

#define OCT_MIN 0
#define OCT_MAX 7                       /* key 26 is then G-9 (115), inside the 0..127 notes */
#define OCT_START 3                     /* key 0 is F3 */

/* envelope, in Q15 per sub-block of SUB frames (CLAUDE.md rule 5) */
#define SUB 16
#define SUB_SHIFT 4
#define ATK_STEP 2048                   /* 16 sub-blocks, 5.8 ms */
#define REL_STEP 128                    /* 256 sub-blocks, 93 ms */

/* written by app_frame only, read by app_audio (32-bit stores, one writer) */
static volatile uint32_t v_inc;         /* phase increment of the note */
static volatile int32_t v_gate;         /* 1 = a key is held */
static volatile int32_t v_master;       /* MASTER, Q12 */

/* audio state, app_audio only */
static phase_t phase;
static int32_t env;                     /* Q15 */
static int32_t gain;                    /* Q15, reached at the end of the last sub-block */

/* ---------------------------------------------------------- interface --- */
static int oct;
static uint8_t order[NKEYS];            /* held keys, oldest first: the last one sounds */
static int held;
static uint32_t prev_buttons, prev_keys;
static int drawn, shown_note, shown_oct;
static uint32_t shown_keys;

static int is_black(int k)
{
    int s = (k + 5) % 12;               /* key 0 is F3 */
    return s == 1 || s == 3 || s == 6 || s == 8 || s == 10;
}

static void draw_keys(uint32_t keys)
{
    int k, white = 0;
    for (k = 0; k < NKEYS; k++)
        if (!is_black(k)) {
            lcd_fill(white * KEY_W, KEYS_Y, KEY_W - 1, 56, keys >> k & 1u ? C_KEY_ON : C_OFF_WHITE);
            white++;
        }
    for (k = 0, white = 0; k < NKEYS; k++) {
        if (is_black(k))
            lcd_fill(white * KEY_W - 5, KEYS_Y, 10, 34, keys >> k & 1u ? C_KEY_ON : C_OFF_BLACK);
        else
            white++;
    }
}

/* "C-4", "C#4", or "---" for no note (note < 0) */
static void note_name(int note, char *s)
{
    static const char NAMES[12][2] = {{'C', '-'}, {'C', '#'}, {'D', '-'}, {'D', '#'}, {'E', '-'},
                                      {'F', '-'}, {'F', '#'}, {'G', '-'}, {'G', '#'}, {'A', '-'},
                                      {'A', '#'}, {'B', '-'}};
    if (note < 0) {
        s[0] = s[1] = s[2] = '-';
    } else {
        s[0] = NAMES[note % 12][0];
        s[1] = NAMES[note % 12][1];
        s[2] = (char)('0' + note / 12);
    }
    s[3] = 0;
}

static void draw(int note, uint32_t keys)
{
    char s[8];
    if (!drawn) {
        lcd_fill(0, 0, 240, 240, C_BG);
        gfx_text(8, 8, "TRAWLER", 2, C_TEXT, C_BG);
        gfx_text(240 - 8 - gfx_text_w("hello", 1), 20, "hello", 1, C_DIM, C_BG);
    }
    if (!drawn || note != shown_note) {
        note_name(note, s);
        gfx_text((240 - gfx_text_w(s, 4)) / 2, 64, s, 4, note < 0 ? C_DIM : C_NOTE, C_BG);
        shown_note = note;
    }
    if (!drawn || oct != shown_oct) {
        s[0] = 'O';
        s[1] = 'C';
        s[2] = 'T';
        s[3] = ' ';
        s[4] = (char)('0' + oct);
        s[5] = 0;
        gfx_text((240 - gfx_text_w(s, 2)) / 2, 126, s, 2, C_TEXT, C_BG);
        shown_oct = oct;
    }
    if (!drawn || keys != shown_keys) {
        draw_keys(keys);
        shown_keys = keys;
    }
    drawn = 1;
}

static void track_keys(uint32_t keys)
{
    uint32_t up = prev_keys & ~keys, down = keys & ~prev_keys;
    int i, j, k;
    for (i = j = 0; i < held; i++)      /* drop the released ones, keep the order */
        if (!(up >> order[i] & 1u))
            order[j++] = order[i];
    held = j;
    for (k = 0; k < NKEYS; k++)
        if (down >> k & 1u && held < NKEYS)
            order[held++] = (uint8_t)k;
}

void app_init(void)
{
    oct = OCT_START;
    held = 0;
    prev_buttons = prev_keys = 0;
    drawn = 0;
    shown_note = -1;
    shown_oct = -1;
    shown_keys = 0;
    v_inc = 0;
    v_gate = 0;
    v_master = plat_master();
    phase = 0;
    env = gain = 0;
}

void app_frame(void)
{
    uint32_t buttons = plat_buttons(), keys = plat_keys();
    uint32_t pressed = buttons & ~prev_buttons, lit = buttons;
    int note = -1;

    if (pressed >> B_OCTDN & 1u && oct > OCT_MIN)
        oct--;
    if (pressed >> B_OCTUP & 1u && oct < OCT_MAX)
        oct++;
    track_keys(keys);
    prev_buttons = buttons;
    prev_keys = keys;

    if (held) {
        note = oct * 12 + 5 + order[held - 1];
        v_inc = note_inc(note, 0);      /* the phase goes on: no click on a note change */
        v_gate = 1;
    } else
        v_gate = 0;                     /* v_inc stays: the release sounds at the same pitch */
    v_master = plat_master();

    if (oct < OCT_START)
        lit |= 1u << B_OCTDN;
    if (oct > OCT_START)
        lit |= 1u << B_OCTUP;
    plat_leds(lit, keys, 0);
    draw(note, keys);
}

/* -------------------------------------------------------------- audio --- */
void app_audio(int32_t *lr, int n)
{
    uint32_t inc = v_inc;
    int32_t gate = v_gate, master = v_master;
    int i, j, m;
    for (j = 0; j < n; j += m) {
        int32_t g0 = gain, g1, g = gain;
        m = n - j < SUB ? n - j : SUB;
        if (gate)
            env = env + ATK_STEP < Q15_ONE ? env + ATK_STEP : Q15_ONE;
        else
            env = env > REL_STEP ? env - REL_STEP : 0;
        g1 = mul_q12(env, master);      /* <= 32767: master <= 4096 */
        /* ramp the gain across the sub-block; -6 dB: a full sine is +-16383 */
        for (i = 0; i < m; i++) {
            int32_t s;
            g = g0 + (((g1 - g0) * (i + 1)) >> SUB_SHIFT);
            s = ((int32_t)sin_q15(phase) * g) >> 16;
            phase += inc;
            lr[2 * (j + i)] = s;
            lr[2 * (j + i) + 1] = s;
        }
        gain = g;
    }
}
