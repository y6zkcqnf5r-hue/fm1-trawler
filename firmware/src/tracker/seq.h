// SPDX-License-Identifier: GPL-3.0-only
/* The sequencer (docs/04_SPEC_TRACKER.md §2): ticks exact to the sample, steps timed by the groove,
 * one voice per track. P1-02 plays one phrase in a loop; chains and the song come with P2.
 *
 * The clock (ADR-023): a tick lasts exactly 110250 / bpm samples (44100 * 60 / 24 ticks per beat).
 * Each tick lasts the quotient, plus one sample whenever the remainder accumulator passes bpm, so
 * tick k falls on sample floor(k * 110250 / bpm): no drift at any tempo. 32-bit division, only when
 * the tempo changes. seq_render() cuts its block at the ticks and handles them between the pieces.
 *
 * The interface side calls seq_cmd_*() (through tracker/cmdq.c); the audio side calls seq_render(),
 * which reads the project (g_proj) and publishes where it plays. */
#pragma once
#include <stdint.h>
#include "app/config.h"

#define SEQ_TICK_NUM 110250u            /* samples per tick * bpm: 44100 * 60 / 24 */
#define SEQ_TPB 24                      /* ticks per beat */

typedef struct { uint32_t q, r, acc, bpm; } sclock_t;
void sclock_set_bpm(sclock_t *c, int bpm);   /* clamped to TRK_TEMPO_MIN..MAX; restarts the remainder */
uint32_t sclock_next(sclock_t *c);           /* length in samples of the next tick */

void seq_init(void);                    /* stopped, voices silent; with the audio not running */

/* interface side: 0, or -1 if the command queue is full */
int seq_cmd_play_phrase(int track, int phrase);   /* loop one phrase on a track, from step 0 */
int seq_cmd_stop(void);
/* play a note on a track's voice, as a key does (note < 0: release it); a step of the playback may
 * take the voice back */
int seq_cmd_preview(int track, int instr, int note);

/* audio side: adds n interleaved L R frames (Q15) to lr */
void seq_render(int32_t *lr, int n);

/* published by the audio side */
extern volatile uint8_t seq_playing;
extern volatile uint8_t seq_row[TRK_TRACKS];      /* the step playing, per track */
