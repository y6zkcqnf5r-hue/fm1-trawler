// SPDX-License-Identifier: GPL-3.0-only
/* P1-04 [PROTO]: the Phrase view drawn by firmware/src/ui/gfx.c in the 6x10 and the 8x12 face, to
 * choose one (Q-02). Not the view itself (P1-05): a fixed picture of the layout of spec §3.
 *
 *   build/host/mockup OUTDIR     writes OUTDIR/phrase_6x10.png and OUTDIR/phrase_8x12.png
 *
 * Native only. It implements lcd_fill / lcd_blit itself, over the same byte-swapped framebuffer as
 * host/trawler_host.c, and saves it with host/png.c. */
#include <stdio.h>
#include <string.h>
#include "host.h"
#include "io.h"
#include "ui/gfx.h"

static uint16_t fb[LCD_W * LCD_H];

void lcd_fill(int x, int y, int w, int h, uint16_t rgb565)
{
    uint16_t c = (uint16_t)((rgb565 >> 8) | (rgb565 << 8));
    int i, j;
    for (j = y; j < y + h; j++)
        for (i = x; i < x + w; i++)
            if (i >= 0 && i < LCD_W && j >= 0 && j < LCD_H)
                fb[j * LCD_W + i] = c;
}

void lcd_blit(int x, int y, int w, int h, const uint16_t *px)
{
    int i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if (x + i >= 0 && x + i < LCD_W && y + j >= 0 && y + j < LCD_H)
                fb[(y + j) * LCD_W + x + i] = px[j * w + i];
}

/* a phrase of the synthwave kind: note, vol, instr, three FX (empty = NULL) */
static const struct { const char *note, *vol, *ins, *fx[3]; } ROWS[16] = {
    {"C-4", "7F", "03", {"ARP37", "CUT40", 0}},
    {0, 0, 0, {0, 0, 0}},
    {"D#4", "60", "03", {"PSL04", 0, 0}},
    {0, 0, 0, {0, 0, 0}},
    {"G-4", 0, "03", {0, 0, "SRV30"}},
    {0, 0, 0, {"VOLF8", 0, 0}},
    {"A#4", "7F", "03", {"ARP3A", 0, 0}},
    {0, 0, 0, {0, 0, 0}},
    {"C-5", "50", "03", {0, "DEL03", 0}},
    {0, 0, 0, {0, 0, 0}},
    {"G-4", 0, 0, {"PSL08", 0, 0}},
    {0, 0, 0, {0, 0, 0}},
    {"D#4", "7F", "03", {"CHA80", 0, 0}},
    {0, 0, 0, {"KIL02", 0, 0}},
    {"OFF", 0, 0, {0, 0, 0}},
    {0, 0, 0, {0, 0, 0}},
};

#define CURSOR_ROW 2
#define PLAY_ROW 6
#define GRID_Y 24
#define ROW_H 12

/* one cell: its text, or dashes in TH_DIM; the cursor's cell in amber */
static int cell(int x, int y, const char *s, int len, uint16_t fg, int cursor, uint16_t bg)
{
    char dash[8];
    int i;
    for (i = 0; i < len; i++)
        dash[i] = '-';
    dash[len] = 0;
    if (cursor)
        gfx_text(x, y, s ? s : dash, 1, TH_BG, TH_CURSOR);
    else
        gfx_text(x, y, s ? s : dash, 1, s ? fg : TH_DIM, bg);
    return x + len * gfx_font_w();
}

static void draw(int font, int gap, int fxgap)
{
    int r, k, cw, ty, x;
    char num[2] = {0, 0};
    static const uint16_t KC[4] = {TH_K1, TH_K2, TH_K3, TH_K4};
    static const char *const KL[4] = {"CUT 40", "RES 20", "SDL 10", "SRV 30"};
    gfx_font(font);
    cw = gfx_font_w();
    ty = (ROW_H - gfx_font_h()) / 2;
    gfx_fill(0, 0, LCD_W, LCD_H, TH_BG);

    /* header: view and number, track, play state and tempo, keyboard octave, mini-map */
    gfx_text(4, 4 + ty, "PHRASE 1A", 1, TH_HEAD, TH_BG);
    gfx_text(4 + 10 * cw + 4, 4 + ty, "T3", 1, TH_TEXT, TH_BG);
    gfx_play_icon(4 + 13 * cw + 4, 6, 8, TH_K3);
    gfx_text(4 + 13 * cw + 14, 4 + ty, "120", 1, TH_TEXT, TH_BG);
    gfx_text(4 + 13 * cw + 22 + 3 * cw, 4 + ty, "O4", 1, TH_DIM, TH_BG);
    for (k = 0; k < 5; k++) {           /* Song Chain Phrase Instr Table: we are in Phrase */
        int bx = LCD_W - 4 - (5 - k) * 9;
        if (k == 2)
            gfx_fill(bx, 6, 7, 8, TH_HEAD);
        else
            gfx_frame(bx, 6, 7, 8, TH_DIM);
    }
    gfx_hline(0, 21, LCD_W, TH_RULE);

    /* the grid: 16 rows of 12 px */
    for (r = 0; r < 16; r++) {
        int y = GRID_Y + r * ROW_H;
        uint16_t bg = r == PLAY_ROW ? TH_PLAYROW : TH_BG;
        if (r == PLAY_ROW)
            gfx_fill(0, y, LCD_W, ROW_H, TH_PLAYROW);
        num[0] = (char)(r < 10 ? '0' + r : 'A' + r - 10);
        x = gap + (font == GFX_FONT_6X10 ? 2 : 0);
        gfx_text(x, y + ty, num, 1, r % 4 ? TH_DIM : TH_HEAD, bg);
        x += cw + gap;
        x = cell(x, y + ty, ROWS[r].note, 3, TH_NOTE, r == CURSOR_ROW, bg) + gap;
        x = cell(x, y + ty, ROWS[r].vol, 2, TH_TEXT, 0, bg) + gap;
        x = cell(x, y + ty, ROWS[r].ins, 2, TH_TEXT, 0, bg) + gap;
        for (k = 0; k < 3; k++) {
            const char *f = ROWS[r].fx[k];
            char cmd[4] = "---", val[3] = "--";
            if (f) {
                memcpy(cmd, f, 3);
                memcpy(val, f + 3, 2);
            }
            x = cell(x, y + ty, f ? cmd : 0, 3, TH_FX, 0, bg) + fxgap;
            x = cell(x, y + ty, f ? val : 0, 2, TH_TEXT, 0, bg) + gap;
        }
    }

    /* footer, two rows of 12 px: the four knob boxes (y 216), a line of help (y 228) */
    for (k = 0; k < 4; k++) {
        int bx = k * 60 + 1, w = gfx_text_w(KL[k], 1);
        gfx_fill(bx, 216, 58, ROW_H, KC[k]);
        gfx_text(bx + (58 - w) / 2, 216 + ty, KL[k], 1, TH_BG, KC[k]);
    }
    gfx_text(4, 228 + ty, "SEL+ALGO>: INSTR 03", 1, TH_DIM, TH_BG);
}

int main(int argc, char **argv)
{
    char path[1024];
    const char *dir = argc > 1 ? argv[1] : ".";
    draw(GFX_FONT_6X10, 9, 3);          /* 40 columns: room between the fields */
    snprintf(path, sizeof path, "%s/phrase_6x10.png", dir);
    if (png_write(path, fb))
        return 1;
    draw(GFX_FONT_8X12, 8, 0);          /* 30 columns: a phrase row is 240 px exactly */
    snprintf(path, sizeof path, "%s/phrase_8x12.png", dir);
    if (png_write(path, fb))
        return 1;
    printf("mockup: %s/phrase_6x10.png, %s/phrase_8x12.png\n", dir, dir);
    return 0;
}
