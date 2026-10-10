// SPDX-License-Identifier: GPL-3.0-only
/* tracker/model.c: empty project and defaults, bounded getters and setters, first free number. */
#include <string.h>
#include "test.h"
#include "tracker/model.h"

static void test_init(void)
{
    int i, r;
    model_init();
    CHECK_EQ(sizeof(project_t), 34160);
    CHECK_EQ(model_rev(), 0);
    CHECK_EQ(g_proj->hdr.version, TRK_FORMAT_VERSION);
    CHECK_EQ(proj_tempo(), 120);
    for (i = 0; i < TRK_NAME_LEN; i++)
        CHECK_EQ(proj_name_get(i), 0);
    for (r = 0; r < TRK_SONG_ROWS; r += 37)
        for (i = 0; i < TRK_TRACKS; i++)
            CHECK_EQ(song_get(r, i), TRK_EMPTY);
    CHECK_EQ(song_get(TRK_SONG_ROWS - 1, TRK_TRACKS - 1), TRK_EMPTY);
    for (r = 0; r < TRK_ROWS; r++) {
        CHECK_EQ(chain_get(TRK_CHAINS - 1, r, CHAIN_PHRASE), TRK_EMPTY);
        CHECK_EQ(chain_get(TRK_CHAINS - 1, r, CHAIN_TRANSP), 0);
    }
    for (i = 0; i < STEP_COLS; i++) {
        int want = i <= STEP_INSTR ? TRK_EMPTY : 0;
        CHECK_EQ(phrase_get(0, 0, i), want);
        CHECK_EQ(phrase_get(TRK_PHRASES - 1, TRK_ROWS - 1, i), want);
    }
    for (i = 0; i < TRK_TABLES; i++) {
        CHECK_EQ(table_tic_get(i), 1);
        CHECK_EQ(table_get(i, 5, TROW_TRANSP), 0);
        CHECK_EQ(table_get(i, 5, TROW_VOL), TRK_EMPTY);
        CHECK_EQ(table_get(i, 5, TROW_FX2), 0);
    }
    for (i = 0; i < TRK_INSTRS; i++) {
        CHECK_EQ(instr_get(i, INSTR_TYPE), INSTR_NONE);
        CHECK_EQ(instr_get(i, INSTR_NAME), 0);
        CHECK_EQ(instr_get(i, INSTR_TABLE), i);      /* table 00-1F belongs to instrument 00-1F */
        CHECK_EQ(instr_get(i, INSTR_TIC), 1);
        CHECK_EQ(instr_get(i, INSTR_BYTES - 1), 0);
    }
    CHECK_EQ(groove_get(0, 0), 6);                   /* groove 0 = 06/06 */
    CHECK_EQ(groove_get(0, 1), 6);
    for (r = 2; r < TRK_ROWS; r++)
        CHECK_EQ(groove_get(0, r), TRK_EMPTY);
    for (i = 1; i < TRK_GROOVES; i++)
        CHECK_EQ(groove_get(i, 0), TRK_EMPTY);
    for (i = 0; i < TRK_TRACKS; i++) {               /* neutral mixer */
        CHECK_EQ(mixer_get(MIX_VOL(i)), 0x80);
        CHECK_EQ(mixer_get(MIX_PAN(i)), 0x80);
    }
    CHECK_EQ(mixer_get(MIX_MUTE), 0);
    CHECK_EQ(mixer_get(MIX_SOLO), 0);
    CHECK_EQ(mixer_get(MIX_MASTER), 0x80);
    CHECK_EQ(mixer_get(MIX_RET_REV), 0x80);
    CHECK_EQ(mixer_get(MIX_CRUSH), 0);
    CHECK_EQ(mixer_get(MIX_LIMIT), 1);
}

/* model_init really clears: dirty the whole project first */
static void test_reinit(void)
{
    model_init();
    memset((void *)g_proj, 0x5A, sizeof(project_t));
    model_init();
    CHECK_EQ(song_get(3, 3), TRK_EMPTY);
    CHECK_EQ(phrase_get(9, 9, STEP_FX1), 0);
    CHECK_EQ(chain_get(9, 9, CHAIN_TRANSP), 0);
    CHECK_EQ(instr_get(9, INSTR_P + 3), 0);
    CHECK_EQ(proj_name_get(4), 0);
    CHECK_EQ(chain_first_free(), 0);
    CHECK_EQ(phrase_first_free(), 0);
}

static void test_bounds(void)
{
    model_init();
    CHECK_EQ(song_get(-1, 0), -1);
    CHECK_EQ(song_get(TRK_SONG_ROWS, 0), -1);
    CHECK_EQ(song_get(0, TRK_TRACKS), -1);
    CHECK_EQ(song_get(0, -1), -1);
    CHECK_EQ(chain_get(TRK_CHAINS, 0, 0), -1);
    CHECK_EQ(chain_get(0, TRK_ROWS, 0), -1);
    CHECK_EQ(chain_get(0, 0, CHAIN_COLS), -1);
    CHECK_EQ(phrase_get(TRK_PHRASES, 0, 0), -1);
    CHECK_EQ(phrase_get(0, -1, 0), -1);
    CHECK_EQ(phrase_get(0, 0, STEP_COLS), -1);
    CHECK_EQ(table_get(TRK_TABLES, 0, 0), -1);
    CHECK_EQ(table_get(0, TRK_ROWS, 0), -1);
    CHECK_EQ(table_get(0, 0, TROW_COLS), -1);
    CHECK_EQ(table_tic_get(-1), -1);
    CHECK_EQ(table_tic_get(TRK_TABLES), -1);
    CHECK_EQ(groove_get(TRK_GROOVES, 0), -1);
    CHECK_EQ(groove_get(0, TRK_ROWS), -1);
    CHECK_EQ(instr_get(TRK_INSTRS, 0), -1);
    CHECK_EQ(instr_get(0, INSTR_BYTES), -1);
    CHECK_EQ(instr_get(0, -1), -1);
    CHECK_EQ(mixer_get(MIX_BYTES), -1);
    CHECK_EQ(mixer_get(-1), -1);
    CHECK_EQ(proj_name_get(TRK_NAME_LEN), -1);

    /* refused writes: out of range index, value not a byte */
    CHECK_EQ(song_set(TRK_SONG_ROWS, 0, 0), -1);
    CHECK_EQ(song_set(0, TRK_TRACKS, 0), -1);
    CHECK_EQ(chain_set(TRK_CHAINS, 0, CHAIN_PHRASE, 0), -1);
    CHECK_EQ(chain_set(0, 0, CHAIN_COLS, 0), -1);
    CHECK_EQ(chain_set(0, 0, CHAIN_TRANSP, 256), -1);
    CHECK_EQ(chain_set(0, 0, CHAIN_TRANSP, -1), -1);
    CHECK_EQ(phrase_set(-1, 0, STEP_VOL, 0), -1);
    CHECK_EQ(phrase_set(0, TRK_ROWS, STEP_VOL, 0), -1);
    CHECK_EQ(phrase_set(0, 0, STEP_COLS, 0), -1);
    CHECK_EQ(phrase_set(0, 0, STEP_VOL, 256), -1);
    CHECK_EQ(table_set(TRK_TABLES, 0, 0, 0), -1);
    CHECK_EQ(table_set(0, 0, TROW_COLS, 0), -1);
    CHECK_EQ(table_tic_set(TRK_TABLES, 1), -1);
    CHECK_EQ(table_tic_set(0, 256), -1);
    CHECK_EQ(groove_set(TRK_GROOVES, 0, 6), -1);
    CHECK_EQ(groove_set(0, TRK_ROWS, 6), -1);
    CHECK_EQ(instr_set(TRK_INSTRS, INSTR_TYPE, 0), -1);
    CHECK_EQ(instr_set(0, INSTR_BYTES, 0), -1);
    CHECK_EQ(mixer_set(MIX_BYTES, 0), -1);
    CHECK_EQ(mixer_set(MIX_VOL(0), -1), -1);
    CHECK_EQ(proj_name_set(TRK_NAME_LEN, 'A'), -1);
    CHECK_EQ(model_rev(), 0);                        /* nothing was written */
}

static void test_values(void)
{
    model_init();
    /* song: a chain number or empty */
    CHECK_EQ(song_set(TRK_SONG_ROWS - 1, TRK_TRACKS - 1, TRK_CHAINS - 1), 0);
    CHECK_EQ(song_get(TRK_SONG_ROWS - 1, TRK_TRACKS - 1), TRK_CHAINS - 1);
    CHECK_EQ(song_set(0, 0, TRK_CHAINS), -1);
    CHECK_EQ(song_set(0, 0, 0xFE), -1);
    CHECK_EQ(song_set(TRK_SONG_ROWS - 1, TRK_TRACKS - 1, TRK_EMPTY), 0);
    CHECK_EQ(song_get(TRK_SONG_ROWS - 1, TRK_TRACKS - 1), TRK_EMPTY);

    /* chain: a phrase number or empty; any transposition, signed */
    CHECK_EQ(chain_set(5, 15, CHAIN_PHRASE, TRK_PHRASES - 1), 0);
    CHECK_EQ(chain_set(5, 15, CHAIN_PHRASE, TRK_PHRASES), -1);
    CHECK_EQ(chain_get(5, 15, CHAIN_PHRASE), TRK_PHRASES - 1);
    CHECK_EQ(chain_set(5, 15, CHAIN_TRANSP, 0xF4), 0);       /* -12 */
    CHECK_EQ(g_proj->chain[5].transp[15], -12);
    CHECK_EQ(chain_get(5, 15, CHAIN_TRANSP), 0xF4);

    /* phrase: note 0..119, OFF or empty; instrument number or empty; the rest any byte */
    CHECK_EQ(phrase_set(1, 2, STEP_NOTE, NOTE_MAX), 0);
    CHECK_EQ(phrase_set(1, 2, STEP_NOTE, NOTE_MAX + 1), -1);
    CHECK_EQ(phrase_set(1, 2, STEP_NOTE, 0xFD), -1);
    CHECK_EQ(phrase_get(1, 2, STEP_NOTE), NOTE_MAX);
    CHECK_EQ(phrase_set(1, 3, STEP_NOTE, NOTE_OFF), 0);
    CHECK_EQ(phrase_set(1, 2, STEP_INSTR, TRK_INSTRS - 1), 0);
    CHECK_EQ(phrase_set(1, 2, STEP_INSTR, TRK_INSTRS), -1);
    CHECK_EQ(phrase_set(1, 2, STEP_VOL, 0xFF), 0);
    CHECK_EQ(phrase_set(1, 2, STEP_FX3, 0x21), 0);
    CHECK_EQ(phrase_set(1, 2, STEP_FX3_VAL, 0xFF), 0);
    CHECK_EQ(g_proj->phrase[1].s[2].fx[2].cmd, 0x21);    /* the column is the byte offset */
    CHECK_EQ(g_proj->phrase[1].s[2].fx[2].val, 0xFF);
    CHECK_EQ(g_proj->phrase[1].s[2].instr, TRK_INSTRS - 1);

    /* table */
    CHECK_EQ(table_set(63, 15, TROW_TRANSP, 0x0C), 0);
    CHECK_EQ(table_set(63, 15, TROW_FX1_VAL, 0x37), 0);
    CHECK_EQ(g_proj->table[63].r[15].transp, 12);
    CHECK_EQ(g_proj->table[63].r[15].fx[0].val, 0x37);
    CHECK_EQ(table_tic_set(63, 4), 0);
    CHECK_EQ(table_tic_get(63), 4);

    /* groove: 0 ticks refused */
    CHECK_EQ(groove_set(1, 0, 0), -1);
    CHECK_EQ(groove_set(1, 0, 8), 0);
    CHECK_EQ(groove_set(1, 1, 4), 0);
    CHECK_EQ(groove_get(1, 1), 4);

    /* instrument: printable name, table number or empty */
    CHECK_EQ(instr_set(2, INSTR_NAME, 'B'), 0);
    CHECK_EQ(instr_set(2, INSTR_NAME + TRK_INSTR_NAME_LEN - 1, '~'), 0);
    CHECK_EQ(instr_set(2, INSTR_NAME + 1, 0x7F), -1);
    CHECK_EQ(instr_set(2, INSTR_NAME + 1, 0x1F), -1);
    CHECK_EQ(g_proj->instr[2].name[0], 'B');
    CHECK_EQ(instr_set(2, INSTR_TABLE, TRK_TABLES), -1);
    CHECK_EQ(instr_set(2, INSTR_TABLE, 0x21), 0);
    CHECK_EQ(g_proj->instr[2].table, 0x21);
    CHECK_EQ(instr_set(2, INSTR_P + 52, 0xAB), 0);
    CHECK_EQ(g_proj->instr[2].p[52], 0xAB);

    /* mixer and header */
    CHECK_EQ(mixer_set(MIX_PAN(7), 0), 0);
    CHECK_EQ(g_proj->mixer.track[7].pan, 0);
    CHECK_EQ(mixer_set(MIX_SOLO, 0x81), 0);
    CHECK_EQ(g_proj->mixer.solo, 0x81);
    CHECK_EQ(mixer_set(MIX_LIMIT, 0), 0);
    CHECK_EQ(g_proj->mixer.limit, 0);
    CHECK_EQ(proj_tempo_set(TRK_TEMPO_MIN - 1), -1);
    CHECK_EQ(proj_tempo_set(TRK_TEMPO_MAX + 1), -1);
    CHECK_EQ(proj_tempo_set(TRK_TEMPO_MIN), 0);
    CHECK_EQ(proj_tempo_set(TRK_TEMPO_MAX), 0);
    CHECK_EQ(proj_tempo(), 300);
    CHECK_EQ(proj_name_set(0, 'T'), 0);
    CHECK_EQ(proj_name_set(1, 0x7F), -1);
    CHECK_EQ(proj_name_get(0), 'T');
}

static void test_rev(void)
{
    uint32_t r;
    model_init();
    CHECK_EQ(phrase_set(0, 0, STEP_NOTE, 48), 0);
    CHECK_EQ(model_rev(), 1);
    CHECK_EQ(phrase_set(0, 0, STEP_NOTE, 48), 0);     /* same value: no change */
    CHECK_EQ(model_rev(), 1);
    CHECK_EQ(phrase_set(0, 0, STEP_NOTE, 200), -1);   /* refused: no change */
    CHECK_EQ(phrase_get(0, 0, STEP_NOTE), 48);
    CHECK_EQ(model_rev(), 1);
    r = model_rev();
    proj_tempo_set(120);
    CHECK_EQ(model_rev(), r);
    proj_tempo_set(121);
    song_set(0, 0, 1);
    chain_set(0, 0, CHAIN_TRANSP, 1);
    table_set(0, 0, TROW_VOL, 1);
    table_tic_set(0, 2);
    groove_set(0, 2, 6);
    instr_set(0, INSTR_TYPE, 0);
    mixer_set(MIX_MUTE, 1);
    proj_name_set(0, 'A');
    CHECK_EQ(model_rev(), r + 9);                    /* each setter goes through touch() */
}

static void test_first_free(void)
{
    int i;
    model_init();
    CHECK_EQ(chain_first_free(), 0);
    CHECK_EQ(phrase_first_free(), 0);
    CHECK_EQ(chain_is_empty(-1), -1);
    CHECK_EQ(phrase_is_empty(TRK_PHRASES), -1);

    /* a phrase with content is taken; so is an empty one that a chain refers to */
    phrase_set(0, 15, STEP_FX1_VAL, 1);
    CHECK_EQ(phrase_is_empty(0), 0);
    CHECK_EQ(phrase_first_free(), 1);
    chain_set(40, 3, CHAIN_PHRASE, 1);
    CHECK_EQ(phrase_is_empty(1), 1);
    CHECK_EQ(phrase_first_free(), 2);
    phrase_set(0, 15, STEP_FX1_VAL, 0);              /* emptied again: free again */
    CHECK_EQ(phrase_first_free(), 0);

    /* chains: chain 40 has a phrase; chain 0 is empty but in the song; a transposition counts */
    CHECK_EQ(chain_is_empty(40), 0);
    song_set(127, 7, 0);
    CHECK_EQ(chain_first_free(), 1);
    chain_set(1, 0, CHAIN_TRANSP, 0xFF);
    CHECK_EQ(chain_first_free(), 2);
    song_set(127, 7, TRK_EMPTY);
    CHECK_EQ(chain_first_free(), 0);

    /* all taken */
    for (i = 0; i < TRK_PHRASES; i++)
        phrase_set(i, 0, STEP_NOTE, NOTE_OFF);
    CHECK_EQ(phrase_first_free(), -1);
    phrase_set(TRK_PHRASES - 1, 0, STEP_NOTE, TRK_EMPTY);
    CHECK_EQ(phrase_first_free(), TRK_PHRASES - 1);
    for (i = 0; i < TRK_CHAINS; i++)
        song_set(i, i % TRK_TRACKS, i);
    CHECK_EQ(chain_first_free(), -1);
}

TEST_MAIN(RUN(test_init); RUN(test_reinit); RUN(test_bounds); RUN(test_values); RUN(test_rev);
          RUN(test_first_free);)
