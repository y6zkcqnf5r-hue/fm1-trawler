// SPDX-License-Identifier: GPL-3.0-only
/* Editing in the Phrase view: see view_phrase.h for the controls (ADR-029). */
#include "tracker/fx.h"
#include "tracker/instr.h"
#include "tracker/model.h"
#include "tracker/seq.h"
#include "ui/view_phrase.h"

/* the FX families, on the white keys 1..6 (spec §6) */
static const uint8_t FAM_FIRST[6] = {FX_ARP, FX_DEL, FX_TBL, FX_VOL, FX_SCH, FX_MDT};
static const uint8_t FAM_LAST[6] = {FX_FIN, FX_HOP, FX_TPO, FX_DRV, FX_SRV, FX_MTP};
#define FAMILIES 6

static uint8_t last[STEP_COLS];         /* the last value of each column: EDIT, PRESETS */
static uint32_t pending_at;
static uint8_t order[NKEYS];            /* held keys, oldest first: the last one sounds */
static int held, sounding;

void view_phrase_init(void)
{
    int c;
    for (c = 0; c < STEP_COLS; c++)
        last[c] = 0;
    last[STEP_NOTE] = 48;               /* C-4 */
    last[STEP_VOL] = 0xFF;
    last[STEP_INSTR] = 0;
    last[STEP_FX1] = last[STEP_FX2] = last[STEP_FX3] = FX_ARP;
    held = 0;
    sounding = -1;
}

void view_phrase_leave(void)
{
    if (sounding >= 0)
        seq_cmd_preview(ui.track, ui.instr, -1);
    sounding = -1;
    held = 0;
}

static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int is_fx(int col) { return col == STEP_FX1 || col == STEP_FX2 || col == STEP_FX3; }
static int is_hex(int col) { return col != STEP_NOTE && !is_fx(col); }
static int get(int col) { return phrase_get(ui.phrase, ui.row, col); }

static int empty(int col)
{
    int v = get(col);
    if (col == STEP_NOTE || col == STEP_VOL || col == STEP_INSTR)
        return v == TRK_EMPTY;
    if (is_fx(col))
        return v == 0;
    return v == 0 && get(col - 1) == 0;  /* a value with no command, and 0 */
}

/* write the cell under the cursor; it becomes the column's last value */
static int put(int col, int v)
{
    if (phrase_set(ui.phrase, ui.row, col, v))
        return -1;
    last[col] = (uint8_t)v;
    if (col == STEP_INSTR)
        ui.instr = (uint8_t)v;
    return 0;
}

/* a note goes with an instrument, as on the M8: the current one, if the step has none */
static void put_note(int note)
{
    if (put(STEP_NOTE, note) == 0 && note <= NOTE_MAX && get(STEP_INSTR) == TRK_EMPTY)
        put(STEP_INSTR, ui.instr);
}

static void commit(void)                /* a lone hex digit is taken as it is */
{
    if (ui.pending >= 0) {
        put(ui.col, ui.pending);
        ui.pending = -1;
    }
}

static int family_of(int code)
{
    int f;
    for (f = 0; f < FAMILIES; f++)
        if (code >= FAM_FIRST[f] && code <= FAM_LAST[f])
            return f;
    return -1;
}

static void edit(int clear)
{
    int c = ui.col, v = get(c);
    if (clear) {
        if (c == STEP_NOTE || c == STEP_VOL || c == STEP_INSTR)
            phrase_set(ui.phrase, ui.row, c, TRK_EMPTY);
        else if (is_fx(c)) {            /* the command and its value */
            phrase_set(ui.phrase, ui.row, c, 0);
            phrase_set(ui.phrase, ui.row, c + 1, 0);
        } else
            phrase_set(ui.phrase, ui.row, c, 0);
        return;
    }
    if (c == STEP_NOTE) {
        if (v == TRK_EMPTY || v == NOTE_OFF)
            put_note(last[STEP_NOTE]);
        else
            put(STEP_NOTE, NOTE_OFF);   /* last stays the note: EDIT again brings it back */
        if (v <= NOTE_MAX)
            last[STEP_NOTE] = (uint8_t)v;
    } else if (empty(c))
        put(c, last[c]);
    else {
        last[c] = (uint8_t)v;           /* picked up: EDIT on an empty cell copies it */
        if (c == STEP_INSTR)
            ui.instr = (uint8_t)v;
    }
}

static void turn_value(int n, int sel)
{
    int c = ui.col, v = get(c);
    if (empty(c) || (c == STEP_NOTE && v == NOTE_OFF)) {
        if (c == STEP_NOTE)
            put_note(last[c]);
        else
            put(c, last[c]);
        return;
    }
    if (c == STEP_NOTE)
        put(c, clamp(v + n * (sel ? 12 : 1), 0, NOTE_MAX));
    else if (c == STEP_INSTR)
        put(c, clamp(v + n * (sel ? 16 : 1), 0, TRK_INSTRS - 1));
    else if (is_fx(c)) {
        if (sel) {                      /* the first command of the next (previous) family */
            int f = clamp(family_of(v) + (n > 0 ? 1 : -1), 0, FAMILIES - 1);
            put(c, FAM_FIRST[f]);
        } else
            put(c, clamp(v + n, 1, FX_COUNT - 1));
    } else
        put(c, clamp(v + n * (sel ? 16 : 1), 0, 0xFF));
}

static void turn_knob(int k, int n, int sel)
{
    static const uint8_t P[4] = {IP_CUT, IP_RES, IP_SDL, IP_SRV};
    int v = instr_get(ui.instr, INSTR_P + P[k]);
    instr_set(ui.instr, INSTR_P + P[k], clamp(v + n * (sel ? 16 : 1), 0, 0xFF));
}

/* the held keys, oldest first; the note the last one plays, or -1 */
static int track_keys(uint32_t keys, uint32_t down)
{
    uint32_t up = ~keys;
    int i, j, k;
    for (i = j = 0; i < held; i++)
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

void view_phrase_input(const input_t *in)
{
    int sel = HELD(in, B_SEL), n, k, note;

    if (ui.pending >= 0 && in->now - pending_at >= UI_HEX_TIMEOUT_MS)
        commit();

    /* the cursor (a move takes a lone digit first) */
    n = in->enc[EN_SELECT];
    if (n) {
        commit();
        ui.row = (uint8_t)clamp(ui.row + n, 0, TRK_ROWS - 1);
    }
    n = in->enc[EN_ALGO];
    if (n) {
        commit();
        ui.col = (uint8_t)clamp(ui.col + n, 0, STEP_COLS - 1);
    }

    if (PRESSED(in, B_EDIT)) {
        commit();
        edit(sel);
    }
    n = in->enc[EN_PRESETS];
    if (n) {
        commit();
        turn_value(n, sel);
    }
    for (k = 0; k < 4; k++)
        if (in->enc[EN_K1 + k])
            turn_knob(k, in->enc[EN_K1 + k], sel);

    /* the keys, by column; with SEL held they are for something else (tracks, actions: P2) */
    note = track_keys(in->keys, in->keys_down);
    if (sel)
        note = -1;
    else if (ui.col == STEP_NOTE) {
        for (k = 0; k < NKEYS; k++)     /* each new key writes its note and moves down */
            if (in->keys_down >> k & 1u) {
                put_note(clamp(ui.oct * 12 + 5 + k, 0, NOTE_MAX));
                ui.row = (uint8_t)clamp(ui.row + ui.step, 0, TRK_ROWS - 1);
            }
    } else {
        note = -1;                      /* the keys do not sound in the other columns */
        for (k = 0; k < NKEYS; k++) {
            int w = ui_white(k);
            if (!(in->keys_down >> k & 1u) || w < 0)
                continue;
            if (is_fx(ui.col)) {
                if (w < FAMILIES) {
                    int v = get(ui.col), f = family_of(v);
                    put(ui.col, f == w && v < FAM_LAST[w] ? v + 1 : FAM_FIRST[w]);
                }
            } else if (ui.pending < 0) {
                ui.pending = (int8_t)w;
                pending_at = in->now;
            } else {
                int v = ui.pending * 16 + w;
                ui.pending = -1;
                if (ui.col == STEP_INSTR && v >= TRK_INSTRS)
                    v = TRK_INSTRS - 1;
                put(ui.col, v);
            }
        }
    }
    if (note != sounding) {
        seq_cmd_preview(ui.track, ui.instr, note);
        sounding = note;
    }
}

uint32_t view_phrase_dim(void)
{
    uint32_t d = 0;
    int w;
    if (is_hex(ui.col))
        for (w = 0; w < 16; w++)
            d |= ui_white_bit(w);
    else if (is_fx(ui.col))
        for (w = 0; w < FAMILIES; w++)
            d |= ui_white_bit(w);
    return d;
}
