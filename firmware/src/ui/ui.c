// SPDX-License-Identifier: GPL-3.0-only
#include "ui/ui.h"
#include "app/plat.h"
#include "tracker/model.h"
#include "tracker/seq.h"
#include "ui/view_phrase.h"

ui_t ui;

static uint32_t prev_buttons, prev_keys;
static uint8_t order[NKEYS];            /* held keys, oldest first: the last one sounds */
static int held, sounding;              /* sounding: the note the keys play, -1 none */
static int drawn;

static const char HEX[] = "0123456789ABCDEF";

void ui_hex2(int v, char *s)
{
    s[0] = HEX[v >> 4 & 15];
    s[1] = HEX[v & 15];
    s[2] = 0;
}

void ui_note(int note, char *s)
{
    static const char NAMES[12][2] = {{'C', '-'}, {'C', '#'}, {'D', '-'}, {'D', '#'}, {'E', '-'},
                                      {'F', '-'}, {'F', '#'}, {'G', '-'}, {'G', '#'}, {'A', '-'},
                                      {'A', '#'}, {'B', '-'}};
    const char *t = 0;
    if (note == TRK_EMPTY)
        t = "---";
    else if (note == NOTE_OFF)
        t = "OFF";
    else if (note < 0 || note > NOTE_MAX)
        t = "???";
    if (t) {
        s[0] = t[0];
        s[1] = t[1];
        s[2] = t[2];
    } else {
        s[0] = NAMES[note % 12][0];
        s[1] = NAMES[note % 12][1];
        s[2] = (char)('0' + note / 12);
    }
    s[3] = 0;
}

void ui_init(void)
{
    ui.view = VIEW_PHRASE;
    ui.track = 0;
    ui.phrase = 0;
    ui.row = ui.col = 0;
    ui.oct = UI_OCT_START;
    ui.instr = 0;
    prev_buttons = prev_keys = 0;
    held = 0;
    sounding = -1;
    drawn = 0;
}

static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

/* the held keys, oldest first; returns the note the last one plays, or -1 */
static int track_keys(uint32_t keys)
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
    if (!held)
        return -1;
    return clamp(ui.oct * 12 + 5 + order[held - 1], 0, NOTE_MAX);   /* key 0 is F */
}

static int white_index(int k)           /* 0..15 for a white key, -1 for a black one */
{
    int s = (k + 5) % 12, w = 0, i;
    if (s == 1 || s == 3 || s == 6 || s == 8 || s == 10)
        return -1;
    for (i = 0; i < k; i++) {
        int t = (i + 5) % 12;
        if (!(t == 1 || t == 3 || t == 6 || t == 8 || t == 10))
            w++;
    }
    return w;
}

static uint32_t white_bit(int w)
{
    int k;
    for (k = 0; k < NKEYS; k++)
        if (white_index(k) == w)
            return 1u << k;
    return 0;
}

void ui_frame(void)
{
    uint32_t buttons = plat_buttons(), keys = plat_keys();
    uint32_t pressed = buttons & ~prev_buttons, lit = buttons, dim = 0;
    int n, note;

    /* the cursor: SELECT moves the row, ALGORITHM the column */
    n = plat_enc(EN_SELECT);
    ui.row = (uint8_t)clamp(ui.row + n, 0, TRK_ROWS - 1);
    n = plat_enc(EN_ALGO);
    ui.col = (uint8_t)clamp(ui.col + n, 0, STEP_COLS - 1);

    if (pressed >> B_OCTDN & 1u && ui.oct > UI_OCT_MIN)
        ui.oct--;
    if (pressed >> B_OCTUP & 1u && ui.oct < UI_OCT_MAX)
        ui.oct++;
    if (pressed >> B_PLAY & 1u) {
        if (seq_playing)
            seq_cmd_stop();
        else
            seq_cmd_play_phrase(ui.track, ui.phrase);
    }

    /* the keys play the note on the track's voice, last one wins (P1-06 writes it) */
    note = track_keys(keys);
    if (note != sounding) {
        seq_cmd_preview(ui.track, ui.instr, note);
        sounding = note;
    }
    prev_buttons = buttons;
    prev_keys = keys;

    lit |= 1u << B_SEQ;                 /* the view's button: Phrase */
    if (seq_playing) {
        lit |= 1u << B_PLAY;
        dim = white_bit(seq_row[ui.track]);   /* the step playing, on the 16 white keys */
    }
    plat_leds(lit, keys, dim);

    view_phrase_draw(!drawn);
    drawn = 1;
}

int ui_probe(int what)
{
    switch (what) {
    case UI_PROBE_VIEW:
        return ui.view;
    case UI_PROBE_TRACK:
        return ui.track;
    case UI_PROBE_PHRASE:
        return ui.phrase;
    case UI_PROBE_ROW:
        return ui.row;
    case UI_PROBE_COL:
        return ui.col;
    case UI_PROBE_OCT:
        return ui.oct;
    }
    return -1;
}
