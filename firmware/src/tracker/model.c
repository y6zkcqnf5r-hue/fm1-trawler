// SPDX-License-Identifier: GPL-3.0-only
/* The project in memory and its only write path (see model.h). */
#include "tracker/model.h"
#include "app/plat.h"

/* in the pool on the device; .pool is not cleared at boot, model_init() does it */
static project_t proj APP_POOL;
const project_t *const g_proj = &proj;

static uint32_t rev;

/* Every write ends here. P2-06 records the old byte here (undo), P2-08 sees the change (autosave).
 * Single stores: the audio path may read the byte, or the tempo, at any time. */
static int touch(uint8_t *p, int v)
{
    if (*p != (uint8_t)v) {
        *p = (uint8_t)v;
        rev++;
    }
    return 0;
}

static int touch16(uint16_t *p, int v)
{
    if (*p != (uint16_t)v) {
        *p = (uint16_t)v;
        rev++;
    }
    return 0;
}

static int in(int i, int n) { return i >= 0 && i < n; }
static int is_byte(int v) { return v >= 0 && v <= 0xFF; }
static int is_ref(int v, int n) { return v == TRK_EMPTY || in(v, n); }
static int is_char(int c) { return c == 0 || (c >= 32 && c <= 126); }

static void fill(void *p, int v, int n)
{
    uint8_t *b = p;
    while (n--)
        *b++ = (uint8_t)v;
}

static void step_clear(step_t *s)
{
    s->note = s->vol = s->instr = TRK_EMPTY;
    fill(s->fx, 0, sizeof s->fx);
}

void model_init(void)
{
    int i, r;

    fill(&proj, 0, sizeof proj);
    proj.hdr.version = TRK_FORMAT_VERSION;
    proj.hdr.tempo = 120;

    for (i = 0; i < TRK_TRACKS; i++)
        proj.mixer.track[i].vol = proj.mixer.track[i].pan = 0x80;
    proj.mixer.ret_cho = proj.mixer.ret_del = proj.mixer.ret_rev = 0x80;
    proj.mixer.cho_rate = 0x20;
    proj.mixer.cho_depth = 0x40;
    proj.mixer.del_time = 0x03;
    proj.mixer.del_fb = 0x60;
    proj.mixer.del_hp = 0x20;
    proj.mixer.rev_size = 0xA0;
    proj.mixer.rev_damp = 0x60;
    proj.mixer.master = 0x80;
    proj.mixer.limit = 0x01;

    fill(proj.song, TRK_EMPTY, sizeof proj.song);
    for (i = 0; i < TRK_CHAINS; i++)
        fill(proj.chain[i].phrase, TRK_EMPTY, sizeof proj.chain[i].phrase);
    for (i = 0; i < TRK_PHRASES; i++)
        for (r = 0; r < TRK_ROWS; r++)
            step_clear(&proj.phrase[i].s[r]);
    for (i = 0; i < TRK_TABLES; i++) {
        for (r = 0; r < TRK_ROWS; r++)
            proj.table[i].r[r].vol = TRK_EMPTY;
        proj.table[i].tic = 1;
    }
    for (i = 0; i < TRK_INSTRS; i++) {
        proj.instr[i].type = INSTR_NONE;
        proj.instr[i].table = (uint8_t)i;
        proj.instr[i].tic = 1;
    }
    fill(proj.groove, TRK_EMPTY, sizeof proj.groove);
    proj.groove[0].ticks[0] = proj.groove[0].ticks[1] = 6;   /* 06/06: sixteenth notes */
    rev = 0;
}

uint32_t model_rev(void) { return rev; }

/* ------------------------------------------------------------- song --- */
int song_get(int row, int track)
{
    if (!in(row, TRK_SONG_ROWS) || !in(track, TRK_TRACKS))
        return -1;
    return proj.song[row][track];
}

int song_set(int row, int track, int chain)
{
    if (!in(row, TRK_SONG_ROWS) || !in(track, TRK_TRACKS) || !is_ref(chain, TRK_CHAINS))
        return -1;
    return touch(&proj.song[row][track], chain);
}

/* ------------------------------------------------------------ chain --- */
static uint8_t *chain_cell(int c, int row, int col)
{
    if (!in(c, TRK_CHAINS) || !in(row, TRK_ROWS) || !in(col, CHAIN_COLS))
        return 0;
    return col == CHAIN_PHRASE ? &proj.chain[c].phrase[row] : (uint8_t *)&proj.chain[c].transp[row];
}

int chain_get(int c, int row, int col)
{
    uint8_t *p = chain_cell(c, row, col);
    return p ? *p : -1;
}

int chain_set(int c, int row, int col, int v)
{
    uint8_t *p = chain_cell(c, row, col);
    if (!p || !is_byte(v) || (col == CHAIN_PHRASE && !is_ref(v, TRK_PHRASES)))
        return -1;
    return touch(p, v);
}

/* ----------------------------------------------------------- phrase --- */
static uint8_t *phrase_cell(int p, int row, int col)
{
    if (!in(p, TRK_PHRASES) || !in(row, TRK_ROWS) || !in(col, STEP_COLS))
        return 0;
    return (uint8_t *)&proj.phrase[p].s[row] + col;
}

int phrase_get(int p, int row, int col)
{
    uint8_t *b = phrase_cell(p, row, col);
    return b ? *b : -1;
}

int phrase_set(int p, int row, int col, int v)
{
    uint8_t *b = phrase_cell(p, row, col);
    if (!b || !is_byte(v))
        return -1;
    if (col == STEP_NOTE && v > NOTE_MAX && v != NOTE_OFF && v != TRK_EMPTY)
        return -1;
    if (col == STEP_INSTR && !is_ref(v, TRK_INSTRS))
        return -1;
    return touch(b, v);
}

/* ------------------------------------------------------------ table --- */
static uint8_t *table_cell(int t, int row, int col)
{
    if (!in(t, TRK_TABLES) || !in(row, TRK_ROWS) || !in(col, TROW_COLS))
        return 0;
    return (uint8_t *)&proj.table[t].r[row] + col;
}

int table_get(int t, int row, int col)
{
    uint8_t *b = table_cell(t, row, col);
    return b ? *b : -1;
}

int table_set(int t, int row, int col, int v)
{
    uint8_t *b = table_cell(t, row, col);
    if (!b || !is_byte(v))
        return -1;
    return touch(b, v);
}

int table_tic_get(int t) { return in(t, TRK_TABLES) ? proj.table[t].tic : -1; }

int table_tic_set(int t, int v)
{
    if (!in(t, TRK_TABLES) || !is_byte(v))
        return -1;
    return touch(&proj.table[t].tic, v);
}

/* ----------------------------------------------------------- groove --- */
int groove_get(int g, int i)
{
    if (!in(g, TRK_GROOVES) || !in(i, TRK_ROWS))
        return -1;
    return proj.groove[g].ticks[i];
}

int groove_set(int g, int i, int v)
{
    /* a 0-tick step would let the sequencer spin without moving on */
    if (!in(g, TRK_GROOVES) || !in(i, TRK_ROWS) || !is_byte(v) || v == 0)
        return -1;
    return touch(&proj.groove[g].ticks[i], v);
}

/* ------------------------------------------------------- instrument --- */
int instr_get(int i, int off)
{
    if (!in(i, TRK_INSTRS) || !in(off, INSTR_BYTES))
        return -1;
    return ((const uint8_t *)&proj.instr[i])[off];
}

int instr_set(int i, int off, int v)
{
    if (!in(i, TRK_INSTRS) || !in(off, INSTR_BYTES) || !is_byte(v))
        return -1;
    if (off >= INSTR_NAME && off < INSTR_NAME + TRK_INSTR_NAME_LEN && !is_char(v))
        return -1;
    if (off == INSTR_TABLE && !is_ref(v, TRK_TABLES))
        return -1;
    return touch((uint8_t *)&proj.instr[i] + off, v);
}

/* ------------------------------------------------------------ mixer --- */
int mixer_get(int off) { return in(off, MIX_BYTES) ? ((const uint8_t *)&proj.mixer)[off] : -1; }

int mixer_set(int off, int v)
{
    if (!in(off, MIX_BYTES) || !is_byte(v))
        return -1;
    return touch((uint8_t *)&proj.mixer + off, v);
}

/* ----------------------------------------------------------- header --- */
int proj_tempo(void) { return proj.hdr.tempo; }

int proj_tempo_set(int bpm)
{
    if (bpm < TRK_TEMPO_MIN || bpm > TRK_TEMPO_MAX)
        return -1;
    return touch16(&proj.hdr.tempo, bpm);
}

int proj_name_get(int i) { return in(i, TRK_NAME_LEN) ? (uint8_t)proj.hdr.name[i] : -1; }

int proj_name_set(int i, int c)
{
    if (!in(i, TRK_NAME_LEN) || !is_char(c))
        return -1;
    return touch((uint8_t *)&proj.hdr.name[i], c);
}

/* ------------------------------------------------------- free numbers --- */
int chain_is_empty(int c)
{
    int r;
    if (!in(c, TRK_CHAINS))
        return -1;
    for (r = 0; r < TRK_ROWS; r++)
        if (proj.chain[c].phrase[r] != TRK_EMPTY || proj.chain[c].transp[r] != 0)
            return 0;
    return 1;
}

int phrase_is_empty(int p)
{
    int r, k;
    if (!in(p, TRK_PHRASES))
        return -1;
    for (r = 0; r < TRK_ROWS; r++) {
        const step_t *s = &proj.phrase[p].s[r];
        if (s->note != TRK_EMPTY || s->vol != TRK_EMPTY || s->instr != TRK_EMPTY)
            return 0;
        for (k = 0; k < 3; k++)
            if (s->fx[k].cmd || s->fx[k].val)
                return 0;
    }
    return 1;
}

int chain_first_free(void)
{
    static uint8_t used[TRK_CHAINS];    /* not on the UI stack */
    int c, r, t;
    fill(used, 0, sizeof used);
    for (r = 0; r < TRK_SONG_ROWS; r++)
        for (t = 0; t < TRK_TRACKS; t++)
            if (proj.song[r][t] < TRK_CHAINS)
                used[proj.song[r][t]] = 1;
    for (c = 0; c < TRK_CHAINS; c++)
        if (!used[c] && chain_is_empty(c))
            return c;
    return -1;
}

int phrase_first_free(void)
{
    static uint8_t used[TRK_PHRASES];
    int p, c, r;
    fill(used, 0, sizeof used);
    for (c = 0; c < TRK_CHAINS; c++)
        for (r = 0; r < TRK_ROWS; r++)
            if (proj.chain[c].phrase[r] < TRK_PHRASES)
                used[proj.chain[c].phrase[r]] = 1;
    for (p = 0; p < TRK_PHRASES; p++)
        if (!used[p] && phrase_is_empty(p))
            return p;
    return -1;
}
