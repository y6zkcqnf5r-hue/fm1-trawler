// SPDX-License-Identifier: GPL-3.0-only
/* The project: song, chains, phrases, tables, instruments, grooves, mixer and header
 * (docs/04_SPEC_TRACKER.md §1). Every cell is one byte, shown in hex; 0xFF is "empty".
 *
 * One static project lives in model.c, in the pool. Readers (the sequencer, on the audio path) read
 * it directly through the const view g_proj. Writers go through the *_set functions below and nothing
 * else: they check the bounds, and they are the one place where undo (P2-06) and autosave (P2-08)
 * see each change (ADR-021). A cell's column number is its byte offset in its struct, so the UI
 * cursor, the setters and the undo journal all speak the same number.
 *
 * Getters return the byte (0..255) or -1 out of range; setters return 0, or -1 if the index or the
 * value is refused (then nothing changes). */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "app/config.h"

#define TRK_EMPTY 0xFF
#define NOTE_OFF 0xFE                   /* shown "OFF": the voice goes to release */
#define NOTE_MAX 119                    /* 0..119 = C-0..B-9 */
#define INSTR_NONE 0xFF                 /* instrument type: no engine (engines come with P1-03) */
#define TRK_FORMAT_VERSION 1            /* project format, bumped when a field changes meaning */

typedef struct { uint8_t cmd, val; } fx_t;                          /* cmd 0 = none */
typedef struct { uint8_t note, vol, instr; fx_t fx[3]; } step_t;    /* 9 B */
typedef struct { step_t s[TRK_ROWS]; } phrase_t;                    /* 144 B */
typedef struct { uint8_t phrase[TRK_ROWS]; int8_t transp[TRK_ROWS]; } chain_t;     /* 32 B */
typedef struct { int8_t transp; uint8_t vol; fx_t fx[3]; } trow_t;  /* 8 B; vol: v/0x80, FF = none */
typedef struct { trow_t r[TRK_ROWS]; uint8_t tic; } table_t;        /* 129 B */
typedef struct { uint8_t ticks[TRK_ROWS]; } groove_t;               /* 16 B; 0xFF = back to the start */
typedef struct {
    uint8_t type;
    char name[TRK_INSTR_NAME_LEN];
    uint8_t table, tic;
    uint8_t p[53];                      /* engine parameters, defined by each engine */
} instr_t;                              /* 64 B */

/* 8-bit gains are v/0x80: 80 = 0 dB, FF = +6 dB. Pan: 00 left, 80 centre, FF right (ADR-022).
 * Per-track sends are on the instrument (SCH SDL SRV); the mixer holds the effect returns and
 * settings. Effect defaults are provisional until P3-05 and P3-06. */
typedef struct { uint8_t vol, pan; } mtrack_t;
typedef struct {
    mtrack_t track[TRK_TRACKS];
    uint8_t mute, solo;                 /* bit t = track t */
    uint8_t ret_cho, ret_del, ret_rev;  /* effect returns */
    uint8_t cho_rate, cho_depth;
    uint8_t del_time, del_fb, del_hp;   /* time in steps (MDT), feedback (MDF), high-pass */
    uint8_t rev_size, rev_damp;         /* size (MRS), damping */
    uint8_t master, tape, crush, limit; /* gain, tape (MTP), bitcrush (MCR, 0 = off), limiter on */
} mixer_t;                              /* 32 B */

typedef struct {
    uint16_t version;                   /* TRK_FORMAT_VERSION */
    uint16_t tempo;                     /* TRK_TEMPO_MIN..TRK_TEMPO_MAX BPM */
    char name[TRK_NAME_LEN];            /* ASCII 32..126, padded with 0 */
} phdr_t;                               /* 16 B */

typedef struct {
    phdr_t hdr;                         /* first: the only 2-byte aligned field */
    mixer_t mixer;
    uint8_t song[TRK_SONG_ROWS][TRK_TRACKS];   /* chain number or empty */
    chain_t chain[TRK_CHAINS];
    phrase_t phrase[TRK_PHRASES];
    table_t table[TRK_TABLES];
    instr_t instr[TRK_INSTRS];
    groove_t groove[TRK_GROOVES];
} project_t;

/* the sizes the spec announces, and the budget (CLAUDE.md rule 4) */
_Static_assert(sizeof(fx_t) == 2, "fx_t");
_Static_assert(sizeof(step_t) == 9, "step_t");
_Static_assert(sizeof(phrase_t) == 144, "phrase_t");
_Static_assert(sizeof(chain_t) == 32, "chain_t");
_Static_assert(sizeof(trow_t) == 8, "trow_t");
_Static_assert(sizeof(table_t) == 129, "table_t");
_Static_assert(sizeof(groove_t) == 16, "groove_t");
_Static_assert(sizeof(instr_t) == 64, "instr_t");
_Static_assert(sizeof(mixer_t) == 32, "mixer_t");
_Static_assert(sizeof(phdr_t) == 16, "phdr_t");
_Static_assert(sizeof(project_t) <= BUDGET_PROJECT, "project_t over BUDGET_PROJECT (48 KiB)");

/* ------------------------------------------- columns = byte offsets --- */
enum { STEP_NOTE, STEP_VOL, STEP_INSTR, STEP_FX1, STEP_FX1_VAL, STEP_FX2, STEP_FX2_VAL, STEP_FX3,
       STEP_FX3_VAL, STEP_COLS };
enum { CHAIN_PHRASE, CHAIN_TRANSP, CHAIN_COLS };
enum { TROW_TRANSP, TROW_VOL, TROW_FX1, TROW_FX1_VAL, TROW_FX2, TROW_FX2_VAL, TROW_FX3, TROW_FX3_VAL,
       TROW_COLS };
enum { INSTR_TYPE = 0, INSTR_NAME = 1, INSTR_TABLE = INSTR_NAME + TRK_INSTR_NAME_LEN, INSTR_TIC,
       INSTR_P, INSTR_BYTES = (int)sizeof(instr_t) };
#define MIX_VOL(t) (2 * (t))
#define MIX_PAN(t) (2 * (t) + 1)
enum { MIX_MUTE = 2 * TRK_TRACKS, MIX_SOLO, MIX_RET_CHO, MIX_RET_DEL, MIX_RET_REV, MIX_CHO_RATE,
       MIX_CHO_DEPTH, MIX_DEL_TIME, MIX_DEL_FB, MIX_DEL_HP, MIX_REV_SIZE, MIX_REV_DAMP, MIX_MASTER,
       MIX_TAPE, MIX_CRUSH, MIX_LIMIT, MIX_BYTES };

_Static_assert(offsetof(step_t, fx[2].val) == STEP_FX3_VAL && sizeof(step_t) == STEP_COLS, "STEP_*");
_Static_assert(offsetof(chain_t, transp) == CHAIN_TRANSP * TRK_ROWS, "CHAIN_*");
_Static_assert(offsetof(trow_t, fx[2].val) == TROW_FX3_VAL && sizeof(trow_t) == TROW_COLS, "TROW_*");
_Static_assert(offsetof(instr_t, table) == INSTR_TABLE && offsetof(instr_t, tic) == INSTR_TIC &&
               offsetof(instr_t, p) == INSTR_P, "INSTR_*");
_Static_assert(offsetof(mixer_t, mute) == MIX_MUTE && offsetof(mixer_t, limit) == MIX_LIMIT &&
               sizeof(mixer_t) == MIX_BYTES, "MIX_*");

/* ------------------------------------------------------------- access --- */
extern const project_t *const g_proj;   /* read-only view of the project */

void model_init(void);                  /* empty project, default values; call before any use */
uint32_t model_rev(void);               /* +1 at every write that changes a byte */

int song_get(int row, int track);
int song_set(int row, int track, int chain);        /* chain < TRK_CHAINS or empty */
int chain_get(int c, int row, int col);             /* CHAIN_*; the transposition as its byte */
int chain_set(int c, int row, int col, int v);      /* phrase < TRK_PHRASES or empty */
int phrase_get(int p, int row, int col);            /* STEP_* */
int phrase_set(int p, int row, int col, int v);     /* note 0..NOTE_MAX, NOTE_OFF or empty; instr < TRK_INSTRS or empty */
int table_get(int t, int row, int col);             /* TROW_* */
int table_set(int t, int row, int col, int v);
int table_tic_get(int t);
int table_tic_set(int t, int v);
int groove_get(int g, int i);
int groove_set(int g, int i, int v);                /* 1..FE ticks, or FF; 0 is refused */
int instr_get(int i, int off);                      /* INSTR_* */
int instr_set(int i, int off, int v);               /* name: 0 or 32..126; table < TRK_TABLES or empty */
int mixer_get(int off);                             /* MIX_* */
int mixer_set(int off, int v);
int proj_tempo(void);
int proj_tempo_set(int bpm);                        /* TRK_TEMPO_MIN..TRK_TEMPO_MAX */
int proj_name_get(int i);
int proj_name_set(int i, int c);                    /* 0 or 32..126 */

int chain_is_empty(int c);              /* 1 if as model_init leaves it, 0, or -1 out of range */
int phrase_is_empty(int p);
int chain_first_free(void);             /* first chain empty and not in the song, or -1 */
int phrase_first_free(void);            /* first phrase empty and in no chain, or -1 */
