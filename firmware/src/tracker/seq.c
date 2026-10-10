// SPDX-License-Identifier: GPL-3.0-only
#include "tracker/seq.h"
#include "tracker/cmdq.h"
#include "tracker/model.h"
#include "dsp/voice.h"

#define STEP_TICKS_DEFAULT 6            /* an empty groove still moves on */

enum { OP_PLAY_PHRASE = 1, OP_STOP = 2 };
#define CMD(op, a, b) ((uint32_t)(op) << 24 | (uint32_t)(a) << 8 | (uint32_t)(b))

volatile uint8_t seq_playing;
volatile uint8_t seq_row[TRK_TRACKS];

/* audio side only */
typedef struct {
    uint8_t on, phrase, row;
    uint8_t groove, gpos;               /* groove number and entry of the step playing */
    uint8_t tick, len;                  /* tick inside the step, ticks of the step */
} track_t;

static track_t trk[TRK_TRACKS];
static voice_t voice[TRK_TRACKS];
static sclock_t clk;
static uint32_t left;                   /* samples until the next tick */
static int playing;

/* ------------------------------------------------------------- clock --- */
void sclock_set_bpm(sclock_t *c, int bpm)
{
    if (bpm < TRK_TEMPO_MIN)
        bpm = TRK_TEMPO_MIN;
    if (bpm > TRK_TEMPO_MAX)
        bpm = TRK_TEMPO_MAX;
    c->bpm = (uint32_t)bpm;
    c->q = SEQ_TICK_NUM / c->bpm;       /* 32-bit, bpm >= 40 */
    c->r = SEQ_TICK_NUM % c->bpm;
    c->acc = 0;
}

uint32_t sclock_next(sclock_t *c)
{
    c->acc += c->r;
    if (c->acc >= c->bpm) {
        c->acc -= c->bpm;
        return c->q + 1;
    }
    return c->q;
}

/* ----------------------------------------------------------- playing --- */
/* ticks of the step at groove entry *pos; FF (or 0) sends back to entry 0 */
static int groove_ticks(int g, uint8_t *pos)
{
    const groove_t *gr = &g_proj->groove[g];
    int v = gr->ticks[*pos];
    if (v == TRK_EMPTY || v == 0) {
        *pos = 0;
        v = gr->ticks[0];
        if (v == TRK_EMPTY || v == 0)
            v = STEP_TICKS_DEFAULT;
    }
    return v;
}

static void trigger(int t)
{
    const step_t *s = &g_proj->phrase[trk[t].phrase].s[trk[t].row];
    voice_t *v = &voice[t];
    if (s->note <= NOTE_MAX) {
        if (s->instr != TRK_EMPTY)
            voice_note(v, s->note, s->vol != TRK_EMPTY ? s->vol : 0xFF);
        else {
            voice_legato(v, s->note);
            if (s->vol != TRK_EMPTY)
                voice_vol(v, s->vol);
        }
    } else if (s->note == NOTE_OFF)
        voice_off(v);
    else if (s->vol != TRK_EMPTY)
        voice_vol(v, s->vol);
}

static void tick(void)
{
    int t;
    for (t = 0; t < TRK_TRACKS; t++) {
        track_t *k = &trk[t];
        if (!k->on)
            continue;
        if (k->tick == 0) {
            k->len = (uint8_t)groove_ticks(k->groove, &k->gpos);
            seq_row[t] = k->row;
            trigger(t);
        }
        if (++k->tick >= k->len) {
            k->tick = 0;
            k->row = (uint8_t)((k->row + 1) & (TRK_ROWS - 1));
            k->gpos = (uint8_t)((k->gpos + 1) & (TRK_ROWS - 1));
        }
    }
    if (g_proj->hdr.tempo != clk.bpm)   /* a new tempo takes effect from the next tick */
        sclock_set_bpm(&clk, g_proj->hdr.tempo);
    left = sclock_next(&clk);
}

static void stop(void)
{
    int t;
    for (t = 0; t < TRK_TRACKS; t++) {
        if (trk[t].on)
            voice_off(&voice[t]);
        trk[t].on = 0;
    }
    playing = 0;
    seq_playing = 0;
}

static void apply(uint32_t c)
{
    int op = (int)(c >> 24), t = (int)(c >> 8 & 0xFF), p = (int)(c & 0xFF);
    if (op == OP_STOP)
        stop();
    else if (op == OP_PLAY_PHRASE && t < TRK_TRACKS && p < TRK_PHRASES) {
        track_t *k = &trk[t];
        k->on = 1;
        k->phrase = (uint8_t)p;
        k->row = k->gpos = k->tick = 0;
        k->groove = 0;
        if (!playing) {                 /* the first tick is on this sample */
            sclock_set_bpm(&clk, g_proj->hdr.tempo);
            left = 0;
            playing = 1;
        }
        seq_playing = 1;
    }
}

void seq_init(void)
{
    int t;
    cmdq_init();
    for (t = 0; t < TRK_TRACKS; t++) {
        voice_reset(&voice[t]);
        trk[t].on = 0;
        seq_row[t] = 0;
    }
    sclock_set_bpm(&clk, 120);
    left = 0;
    playing = 0;
    seq_playing = 0;
}

int seq_cmd_play_phrase(int track, int phrase)
{
    if (track < 0 || track >= TRK_TRACKS || phrase < 0 || phrase >= TRK_PHRASES)
        return -1;
    return cmdq_push(CMD(OP_PLAY_PHRASE, track, phrase));
}

int seq_cmd_stop(void) { return cmdq_push(CMD(OP_STOP, 0, 0)); }

void seq_render(int32_t *lr, int n)
{
    uint32_t c;
    int t;
    while (cmdq_pop(&c))
        apply(c);
    while (n > 0) {
        int k = n;
        if (playing) {
            if (left == 0)
                tick();
            if ((uint32_t)k > left)
                k = (int)left;
        }
        for (t = 0; t < TRK_TRACKS; t++)
            if (voice_active(&voice[t]))
                voice_render(&voice[t], lr, k);
        lr += 2 * k;
        n -= k;
        if (playing)
            left -= (uint32_t)k;
    }
}
