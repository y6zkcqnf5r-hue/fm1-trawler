// SPDX-License-Identifier: GPL-3.0-only
/* Trawler limits and memory budgets, in one place.
 *
 * The TRK_* limits size the project (docs/04_SPEC_TRACKER.md §1); they may shrink if memory runs
 * short, but a saved project stores object numbers, so they never shrink after P2-08 without a new
 * format version. The BUDGET_* figures are the FM-1's (CLAUDE.md rule 4): tracker/model.h checks the
 * project against BUDGET_PROJECT at compile time, and tools/size_report.py reads all four from this
 * file (keep the "(N * 1024)" form, the script parses it). */
#pragma once

/* ---------------------------------------------------------- project --- */
#define TRK_TRACKS 8
#define TRK_SONG_ROWS 128
#define TRK_CHAINS 128
#define TRK_PHRASES 128
#define TRK_INSTRS 32
#define TRK_TABLES 64                   /* 00-1F: default table of the instrument of the same number */
#define TRK_GROOVES 16
#define TRK_ROWS 16                     /* rows of a chain, phrase, table or groove */
#define TRK_NAME_LEN 12                 /* project name */
#define TRK_INSTR_NAME_LEN 8
#define TRK_TEMPO_MIN 40
#define TRK_TEMPO_MAX 300

/* ---------------------------------------------------------- budgets --- */
#define BUDGET_RAM (96 * 1024)          /* .data + .bss */
#define BUDGET_POOL (336 * 1024)        /* .pool: buffers marked APP_POOL */
#define BUDGET_CODE (560 * 1024)        /* .text + .rodata + .data image, in flash */
#define BUDGET_PROJECT (48 * 1024)      /* project_t in memory, and a serialized project */
