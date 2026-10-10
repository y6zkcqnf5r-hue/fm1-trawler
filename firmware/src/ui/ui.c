// SPDX-License-Identifier: GPL-3.0-only
#include "ui/ui.h"
#include "tracker/model.h"
#include "tracker/seq.h"
#include "ui/view_phrase.h"
#include "ui/view_project.h"

ui_t ui;

static uint32_t prev_buttons, prev_keys;
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

static int is_black(int k)
{
    int s = (k + 5) % 12;               /* key 0 is F3 */
    return s == 1 || s == 3 || s == 6 || s == 8 || s == 10;
}

int ui_white(int key)
{
    int i, w = 0;
    if (key < 0 || key >= NKEYS || is_black(key))
        return -1;
    for (i = 0; i < key; i++)
        w += !is_black(i);
    return w;
}

uint32_t ui_white_bit(int w)
{
    int k;
    for (k = 0; k < NKEYS; k++)
        if (ui_white(k) == w)
            return 1u << k;
    return 0;
}

void ui_init(void)
{
    ui.view = VIEW_PHRASE;
    ui.track = 0;
    ui.phrase = 0;
    ui.row = ui.col = 0;
    ui.oct = UI_OCT_START;
    ui.step = 1;
    ui.instr = 0;
    ui.pending = -1;
    prev_buttons = prev_keys = 0;
    drawn = 0;
    view_phrase_init();
}

void ui_frame(void)
{
    input_t in;
    uint32_t lit, dim;
    int e;

    in.held = plat_buttons();
    in.pressed = in.held & ~prev_buttons;
    in.keys = plat_keys();
    in.keys_down = in.keys & ~prev_keys;
    for (e = 0; e < EN_COUNT; e++)
        in.enc[e] = plat_enc(e);
    in.now = plat_ms();
    prev_buttons = in.held;
    prev_keys = in.keys;

    /* shared by every view: OCT-/OCT+ the keyboard octave, SEL+OCT-/OCT+ the edit step; PLAY */
    if (PRESSED(&in, B_OCTDN)) {
        if (HELD(&in, B_SEL))
            ui.step = (uint8_t)(ui.step > 0 ? ui.step - 1 : 0);
        else if (ui.oct > UI_OCT_MIN)
            ui.oct--;
    }
    if (PRESSED(&in, B_OCTUP)) {
        if (HELD(&in, B_SEL))
            ui.step = (uint8_t)(ui.step < UI_STEP_MAX ? ui.step + 1 : UI_STEP_MAX);
        else if (ui.oct < UI_OCT_MAX)
            ui.oct++;
    }
    if (PRESSED(&in, B_PLAY)) {
        if (seq_playing)
            seq_cmd_stop();
        else
            seq_cmd_play_phrase(ui.track, ui.phrase);
    }

    /* the view buttons: SEQ Phrase, GLO Project (Groove first from P2-05) */
    if (PRESSED(&in, B_SEQ) && ui.view != VIEW_PHRASE) {
        ui.view = VIEW_PHRASE;
        drawn = 0;
    } else if (PRESSED(&in, B_GLO) && ui.view != VIEW_PROJECT) {
        view_phrase_leave();
        ui.view = VIEW_PROJECT;
        drawn = 0;
    }

    if (ui.view == VIEW_PHRASE)
        view_phrase_input(&in);
    else
        view_project_input(&in);

    lit = in.held | 1u << (ui.view == VIEW_PHRASE ? B_SEQ : B_GLO);   /* the view's button */
    dim = ui.view == VIEW_PHRASE ? view_phrase_dim() : 0;   /* what the keys do in this column */
    if (seq_playing) {
        lit |= 1u << B_PLAY;
        dim = ui_white_bit(seq_row[ui.track]);   /* the step playing, on the 16 white keys */
    }
    plat_leds(lit, in.keys, dim);

    if (ui.view == VIEW_PHRASE)
        view_phrase_draw(!drawn);
    else
        view_project_draw(!drawn);
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
    case UI_PROBE_STEP:
        return ui.step;
    case UI_PROBE_INSTR:
        return ui.instr;
    case UI_PROBE_PENDING:
        return ui.pending;
    case UI_PROBE_FIELD:
        return view_project_probe(PROJ_PROBE_FIELD);
    case UI_PROBE_CHAR:
        return view_project_probe(PROJ_PROBE_CHAR);
    }
    return -1;
}
