// SPDX-License-Identifier: GPL-3.0-only
#include "ui/view_phrase.h"
#include "tracker/fx.h"
#include "tracker/instr.h"
#include "tracker/model.h"
#include "tracker/seq.h"
#include "ui/gfx.h"
#include "ui/ui.h"

#define W 240
#define ROW_H 12
#define HEAD_Y 0
#define GRID_Y 24
#define FOOT_Y 216                      /* knob boxes, then the help line at FOOT_Y + ROW_H */

/* The face and its spacing (Q-02: 8x12 for now; for 6x10, LAYOUT = 1, then make golden) */
static const struct { int font, gap, fxgap, pad; } LAYOUTS[2] = {
    {GFX_FONT_8X12, 8, 0, 0},           /* 30 columns, a phrase row is 240 px */
    {GFX_FONT_6X10, 9, 3, 2},           /* 40 columns, more air between the fields */
};
#define LAYOUT 0

static const char *const COL_NAMES[STEP_COLS] = {"NOTE", "VOL", "INSTR", "FX1", "FX1 VAL",
                                                  "FX2", "FX2 VAL", "FX3", "FX3 VAL"};

/* what is on the screen, to redraw only what changed */
static struct {
    uint8_t cells[TRK_ROWS][STEP_COLS];
    uint8_t cursor[TRK_ROWS];           /* the cursor's column on that row, 0xFF none */
    int8_t pend[TRK_ROWS];              /* the hex digit pending in the cursor's cell, -1 none */
    uint8_t play[TRK_ROWS];
    int phrase, track, playing, tempo, oct;
    uint8_t knobs[4];
    char help[24];
} shown;

static int cw, ty;                      /* cell width, text offset inside a 12 px row */
static int xs[STEP_COLS + 1];           /* x of the row number (index STEP_COLS) and each column */

static void layout(void)
{
    int g = LAYOUTS[LAYOUT].gap, f = LAYOUTS[LAYOUT].fxgap, x, k;
    gfx_font(LAYOUTS[LAYOUT].font);
    cw = gfx_font_w();
    ty = (ROW_H - gfx_font_h()) / 2;
    x = g + LAYOUTS[LAYOUT].pad;
    xs[STEP_COLS] = x;
    x += cw + g;
    xs[STEP_NOTE] = x;
    x += 3 * cw + g;
    xs[STEP_VOL] = x;
    x += 2 * cw + g;
    xs[STEP_INSTR] = x;
    x += 2 * cw + g;
    for (k = 0; k < 3; k++) {
        xs[STEP_FX1 + 2 * k] = x;
        x += 3 * cw + f;
        xs[STEP_FX1_VAL + 2 * k] = x;
        x += 2 * cw + g;
    }
}

/* the text of one cell */
static void cell_text(const step_t *s, int col, char *t)
{
    const uint8_t *b = (const uint8_t *)s;
    int v = b[col];
    if (col == STEP_NOTE)
        ui_note(v, t);
    else if (col == STEP_FX1 || col == STEP_FX2 || col == STEP_FX3) {
        const char *n = fx_name(v);
        t[0] = n[0];
        t[1] = n[1];
        t[2] = n[2];
        t[3] = 0;
    } else if ((col == STEP_VOL || col == STEP_INSTR) && v == TRK_EMPTY) {
        t[0] = t[1] = '-';
        t[2] = 0;
    } else if (col != STEP_VOL && col != STEP_INSTR && v == 0 && b[col - 1] == 0) {
        t[0] = t[1] = '-';              /* an FX value with no command */
        t[2] = 0;
    } else
        ui_hex2(v, t);
}

static int empty_cell(const step_t *s, int col)
{
    const uint8_t *b = (const uint8_t *)s;
    if (col == STEP_NOTE || col == STEP_VOL || col == STEP_INSTR)
        return b[col] == TRK_EMPTY;
    if (col == STEP_FX1 || col == STEP_FX2 || col == STEP_FX3)
        return b[col] == 0;
    return b[col] == 0 && b[col - 1] == 0;
}

static void draw_row(int r, const step_t *s, int cursor, int play)
{
    uint16_t bg = play ? TH_PLAYROW : TH_BG;
    char t[4];
    int c, y = GRID_Y + r * ROW_H;
    gfx_fill(0, y, W, ROW_H, bg);
    t[0] = "0123456789ABCDEF"[r];
    t[1] = 0;
    gfx_text(xs[STEP_COLS], y + ty, t, 1, r % 4 ? TH_DIM : TH_HEAD, bg);
    for (c = 0; c < STEP_COLS; c++) {
        uint16_t fg = c == STEP_NOTE ? TH_NOTE : (c == STEP_FX1 || c == STEP_FX2 || c == STEP_FX3) ? TH_FX : TH_TEXT;
        cell_text(s, c, t);
        if (c == cursor && ui.pending >= 0) {   /* "7_": the second digit is awaited */
            t[0] = "0123456789ABCDEF"[ui.pending];
            t[1] = '_';
            t[2] = 0;
        }
        if (c == cursor)
            gfx_text(xs[c], y + ty, t, 1, TH_BG, TH_CURSOR);
        else
            gfx_text(xs[c], y + ty, t, 1, empty_cell(s, c) ? TH_DIM : fg, bg);
    }
}

static void num3(int v, char *s)        /* 40..300, right-aligned in 3 */
{
    s[0] = v >= 100 ? (char)('0' + v / 100) : ' ';
    s[1] = (char)('0' + v / 10 % 10);
    s[2] = (char)('0' + v % 10);
    s[3] = 0;
}

static void draw_header(int playing, int tempo)
{
    char s[12];
    int k, x;
    gfx_fill(0, HEAD_Y, W, 20, TH_BG);
    gfx_text(4, 4 + ty, "PHRASE", 1, TH_HEAD, TH_BG);
    ui_hex2(ui.phrase, s);
    gfx_text(4 + 7 * cw, 4 + ty, s, 1, TH_HEAD, TH_BG);
    x = 4 + 10 * cw + 4;
    s[0] = 'T';
    s[1] = (char)('1' + ui.track);
    s[2] = 0;
    gfx_text(x, 4 + ty, s, 1, TH_TEXT, TH_BG);
    x += 3 * cw;
    if (playing)
        gfx_play_icon(x, 6, 8, TH_K3);
    else
        gfx_fill(x, 7, 6, 6, TH_DIM);
    x += 10;
    num3(tempo, s);
    gfx_text(x, 4 + ty, s, 1, TH_TEXT, TH_BG);
    x += 3 * cw + 8;
    s[0] = 'O';                         /* OCT, not O: in 8x12, "O3" reads "03" */
    s[1] = 'C';
    s[2] = 'T';
    s[3] = (char)('0' + ui.oct);
    s[4] = 0;
    gfx_text(x, 4 + ty, s, 1, TH_DIM, TH_BG);
    for (k = 0; k < 5; k++) {           /* mini-map: Song Chain Phrase Instr Table */
        int bx = W - 4 - (5 - k) * 9;
        if (k == 2)
            gfx_fill(bx, 6, 7, 8, TH_HEAD);
        else
            gfx_frame(bx, 6, 7, 8, TH_DIM);
    }
    gfx_hline(0, 21, W, TH_RULE);
}

static void draw_footer(const uint8_t *knobs, const char *help)
{
    static const uint16_t KC[4] = {TH_K1, TH_K2, TH_K3, TH_K4};
    static const char KN[4][4] = {"CUT", "RES", "SDL", "SRV"};
    char s[8];
    int k;
    for (k = 0; k < 4; k++) {
        int bx = k * 60 + 1, w;
        s[0] = KN[k][0];
        s[1] = KN[k][1];
        s[2] = KN[k][2];
        s[3] = ' ';
        ui_hex2(knobs[k], s + 4);
        w = gfx_text_w(s, 1);
        gfx_fill(bx, FOOT_Y, 58, ROW_H, KC[k]);
        gfx_text(bx + (58 - w) / 2, FOOT_Y + ty, s, 1, TH_BG, KC[k]);
    }
    gfx_fill(0, FOOT_Y + ROW_H, W, ROW_H, TH_BG);
    gfx_text(4, FOOT_Y + ROW_H + ty, help, 1, TH_DIM, TH_BG);
}

static int str_eq(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static void str_cpy(char *d, const char *s, int n)
{
    while (--n > 0 && *s)
        *d++ = *s++;
    *d = 0;
}

void view_phrase_draw(int full)
{
    const phrase_t *ph = &g_proj->phrase[ui.phrase];
    const instr_t *in = &g_proj->instr[ui.instr];
    int playing = seq_playing, prow = playing ? seq_row[ui.track] : -1, tempo = g_proj->hdr.tempo;
    uint8_t knobs[4];
    char help[24], v[4];
    int r, c, k;

    if (full) {
        layout();
        gfx_fill(0, 0, W, 240, TH_BG);
    }
    if (full || shown.phrase != ui.phrase || shown.track != ui.track || shown.playing != playing ||
        shown.tempo != tempo || shown.oct != ui.oct) {
        draw_header(playing, tempo);
        shown.phrase = ui.phrase;
        shown.track = ui.track;
        shown.playing = playing;
        shown.tempo = tempo;
        shown.oct = ui.oct;
    }
    for (r = 0; r < TRK_ROWS; r++) {
        const uint8_t *b = (const uint8_t *)&ph->s[r];
        int cur = r == ui.row ? ui.col : 0xFF, pend = r == ui.row ? ui.pending : -1;
        int same = !full && shown.cursor[r] == cur && shown.play[r] == (r == prow) && shown.pend[r] == pend;
        for (c = 0; c < STEP_COLS && same; c++)
            same = shown.cells[r][c] == b[c];
        if (same)
            continue;
        draw_row(r, &ph->s[r], cur, r == prow);
        for (c = 0; c < STEP_COLS; c++)
            shown.cells[r][c] = b[c];
        shown.cursor[r] = (uint8_t)cur;
        shown.pend[r] = (int8_t)pend;
        shown.play[r] = (uint8_t)(r == prow);
    }

    knobs[0] = in->p[IP_CUT];
    knobs[1] = in->p[IP_RES];
    knobs[2] = in->p[IP_SDL];
    knobs[3] = in->p[IP_SRV];
    str_cpy(help, COL_NAMES[ui.col], sizeof help);
    for (k = 0; help[k]; k++)
        ;
    help[k++] = ' ';
    cell_text(&ph->s[ui.row], ui.col, v);
    if (ui.pending >= 0) {
        v[0] = "0123456789ABCDEF"[ui.pending];
        v[1] = '_';
        v[2] = 0;
    }
    str_cpy(help + k, v, (int)sizeof help - k);
    if (ui.col == STEP_NOTE) {          /* the edit step: rows down after a note */
        for (k = 0; help[k]; k++)
            ;
        str_cpy(help + k, "  STEP ", (int)sizeof help - k);
        k += 7;
        if (k < (int)sizeof help - 2) {
            help[k] = "0123456789ABCDEF"[ui.step];
            help[k + 1] = 0;
        }
    }
    for (k = 0; k < 4 && !full && shown.knobs[k] == knobs[k]; k++)
        ;
    if (full || k < 4 || !str_eq(shown.help, help)) {
        draw_footer(knobs, help);
        for (k = 0; k < 4; k++)
            shown.knobs[k] = knobs[k];
        str_cpy(shown.help, help, sizeof shown.help);
    }
}
