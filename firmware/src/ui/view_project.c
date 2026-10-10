// SPDX-License-Identifier: GPL-3.0-only
#include "ui/view_project.h"
#include "tracker/model.h"
#include "tracker/seq.h"
#include "ui/gfx.h"

#define W 240
#define ROW_H 12
#define LIST_Y 40

enum { F_TEMPO, F_NAME, FIELDS };

static const char CHARSET[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.#";
#define NCHARS ((int)sizeof CHARSET - 1)

static int field, pos;                  /* the cursor: field, and character in NAME */
static struct {
    int field, pos, tempo, playing;
    char name[TRK_NAME_LEN];
} shown;

static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static int char_index(int c)
{
    int i;
    for (i = 0; i < NCHARS; i++)
        if (CHARSET[i] == c)
            return i;
    return 0;                           /* 0 (unset) and the others count as a space */
}

void view_project_input(const input_t *in)
{
    int sel = HELD(in, B_SEL), n;
    n = in->enc[EN_SELECT];
    if (n)
        field = clamp(field + n, 0, FIELDS - 1);
    n = in->enc[EN_ALGO];
    if (n && field == F_NAME)
        pos = clamp(pos + n, 0, TRK_NAME_LEN - 1);
    n = in->enc[EN_PRESETS];
    if (n) {
        if (field == F_TEMPO)
            proj_tempo_set(clamp(proj_tempo() + n * (sel ? 10 : 1), TRK_TEMPO_MIN, TRK_TEMPO_MAX));
        else {
            int i = char_index(proj_name_get(pos)) + n * (sel ? 10 : 1);
            i = ((i % NCHARS) + NCHARS) % NCHARS;   /* 32-bit, NCHARS constant */
            proj_name_set(pos, CHARSET[i]);
        }
    }
    if (PRESSED(in, B_EDIT) && sel && field == F_NAME)
        proj_name_set(pos, 0);
}

static void num3(int v, char *s)
{
    s[0] = v >= 100 ? (char)('0' + v / 100) : ' ';
    s[1] = (char)('0' + v / 10 % 10);
    s[2] = (char)('0' + v % 10);
    s[3] = 0;
}

static void draw_field(int f, int ty, int cw)
{
    int y = LIST_Y + f * 2 * ROW_H, i;
    char s[4];
    gfx_fill(0, y, W, ROW_H, TH_BG);
    gfx_text(8, y + ty, f == F_TEMPO ? "TEMPO" : "NAME", 1, f == field ? TH_HEAD : TH_DIM, TH_BG);
    if (f == F_TEMPO) {
        num3(proj_tempo(), s);
        gfx_text(8 + 8 * cw, y + ty, s, 1, f == field ? TH_BG : TH_TEXT, f == field ? TH_CURSOR : TH_BG);
        gfx_text(8 + 12 * cw, y + ty, "BPM", 1, TH_DIM, TH_BG);
        return;
    }
    for (i = 0; i < TRK_NAME_LEN; i++) {
        int c = proj_name_get(i), cur = f == field && i == pos;
        s[0] = (char)(c ? c : '_');
        s[1] = 0;
        gfx_text(8 + (8 + i) * cw, y + ty, s, 1, cur ? TH_BG : c ? TH_TEXT : TH_DIM, cur ? TH_CURSOR : TH_BG);
    }
}

void view_project_draw(int full)
{
    int cw, ty, playing = seq_playing, changed, i;
    gfx_font(GFX_FONT_8X12);
    cw = gfx_font_w();
    ty = (ROW_H - gfx_font_h()) / 2;
    if (full) {
        gfx_fill(0, 0, W, 240, TH_BG);
        gfx_text(4, 4 + ty, "PROJECT", 1, TH_HEAD, TH_BG);
        gfx_hline(0, 21, W, TH_RULE);
        gfx_text(8, 216 + ty, "SLOTS, LOAD, SAVE: P2", 1, TH_DIM, TH_BG);
        gfx_text(8, 228 + ty, "SEQ: BACK TO PHRASE", 1, TH_DIM, TH_BG);
    }
    if (full || playing != shown.playing) {
        if (playing)
            gfx_play_icon(W - 16, 6, 8, TH_K3);
        else {
            gfx_fill(W - 16, 6, 8, 8, TH_BG);
            gfx_fill(W - 16, 7, 6, 6, TH_DIM);
        }
        shown.playing = playing;
    }
    changed = full || shown.field != field || shown.pos != pos;
    if (changed || shown.tempo != proj_tempo())
        draw_field(F_TEMPO, ty, cw);
    for (i = 0; i < TRK_NAME_LEN && !changed; i++)
        changed = shown.name[i] != proj_name_get(i);
    if (changed)
        draw_field(F_NAME, ty, cw);
    shown.field = field;
    shown.pos = pos;
    shown.tempo = proj_tempo();
    for (i = 0; i < TRK_NAME_LEN; i++)
        shown.name[i] = (char)proj_name_get(i);
}

int view_project_probe(int what) { return what == PROJ_PROBE_FIELD ? field : pos; }
