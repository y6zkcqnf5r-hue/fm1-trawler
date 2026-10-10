// SPDX-License-Identifier: GPL-3.0-only
/* tracker/seq.c, tracker/cmdq.c, dsp/voice.c: the tick clock against an exact 64-bit reference,
 * steps on the right sample, the same sound however the blocks are cut, the 2 ms fade. */
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "tracker/cmdq.h"
#include "tracker/model.h"
#include "tracker/seq.h"
#include "dsp/voice.h"

#define FS 44100
#define MAXF (10 * FS)
#define REF_NUM ((uint64_t)FS * 60 / 24)          /* samples per tick * bpm, from first principles */

static int32_t buf[2 * MAXF], buf2[2 * MAXF];

/* render n frames from a fresh start of phrase 0 on track 0, in blocks of `block` (0 = varied) */
static void render(int32_t *out, int n, int block)
{
    uint32_t lcg = 12345;
    int done = 0;
    memset(out, 0, (size_t)n * 2 * sizeof *out);
    seq_init();
    seq_cmd_play_phrase(0, 0);
    while (done < n) {
        int k = block;
        if (!k) {
            lcg = lcg * 1103515245u + 12345u;
            k = 1 + (int)(lcg >> 16) % 300;
        }
        if (k > n - done)
            k = n - done;
        seq_render(out + 2 * done, k);
        done += k;
    }
}

static int first_sound(const int32_t *out, int n)
{
    int i;
    for (i = 0; i < n; i++)
        if (out[2 * i] || out[2 * i + 1])
            return i;
    return -1;
}

static void one_note(int step)
{
    model_init();
    phrase_set(0, step, STEP_NOTE, 48);
    phrase_set(0, step, STEP_INSTR, 0);
}

/* every tick, for 10 minutes, at every tempo, on floor(k * 110250 / bpm) */
static void test_clock(void)
{
    int bpm, worst = 0, ticks_120 = 0;
    sclock_t c;
    for (bpm = TRK_TEMPO_MIN; bpm <= TRK_TEMPO_MAX; bpm++) {
        uint64_t pos = 0, k = 0;
        sclock_set_bpm(&c, bpm);
        while (pos < 600ull * FS) {
            int64_t e = (int64_t)pos - (int64_t)(k * REF_NUM / (uint64_t)bpm);
            if (e < 0)
                e = -e;
            if (e > worst)
                worst = (int)e;
            pos += sclock_next(&c);
            k++;
        }
        if (bpm == 120)
            ticks_120 = (int)k;
    }
    CHECK_EQ(worst, 0);
    CHECK_EQ(SEQ_TICK_NUM, REF_NUM);
    CHECK_EQ(ticks_120, 600 * 2 * SEQ_TPB);          /* 10 min at 120 BPM: 1200 beats */
    printf("  clock: 40..300 BPM, 10 min each, worst error %d sample(s)\n", worst);
    sclock_set_bpm(&c, 120);
    CHECK_EQ(c.q, 918);                              /* 918.75 samples per tick */
    CHECK_EQ(c.r, 90);
    sclock_set_bpm(&c, 10);                          /* clamped: no division by a tiny tempo */
    CHECK_EQ(c.bpm, TRK_TEMPO_MIN);
}

/* a step starts on the very sample of its tick (the sine starts at 0, sound on the next one) */
static void test_trigger(void)
{
    one_note(4);
    render(buf, 30000, 256);
    CHECK_EQ(first_sound(buf, 30000), 22050 + 1);     /* tick 24 = 24 * 918.75 */
    proj_tempo_set(133);
    render(buf, 30000, 256);
    CHECK_EQ(first_sound(buf, 30000), 24 * 110250 / 133 + 1);
    proj_tempo_set(120);
    groove_set(0, 0, 4);                             /* groove 04 FF: 4-tick steps */
    groove_set(0, 1, TRK_EMPTY);
    render(buf, 30000, 256);
    CHECK_EQ(first_sound(buf, 30000), 16 * 110250 / 120 + 1);
    groove_set(0, 0, TRK_EMPTY);                     /* empty groove: 6 ticks */
    render(buf, 30000, 256);
    CHECK_EQ(first_sound(buf, 30000), 22050 + 1);
    one_note(0);                                     /* step 0: the first sample of the block */
    render(buf, 1000, 256);
    CHECK_EQ(first_sound(buf, 1000), 1);
}

/* the loop: a phrase is 16 * 6 ticks = 2 s at 120 BPM; after OFF and the release, silence */
static void test_loop_and_off(void)
{
    int i, quiet = 1;
    one_note(0);
    phrase_set(0, 14, STEP_NOTE, NOTE_OFF);          /* tick 84: sample 77175 */
    render(buf, 3 * 88200, 256);
    CHECK(buf[2 * 77000] != 0 || buf[2 * 77001] != 0);
    for (i = 77175 + 4096 + 32; i < 88200; i++)
        if (buf[2 * i] || buf[2 * i + 1])
            quiet = 0;
    CHECK(quiet);
    CHECK_EQ(first_sound(buf + 2 * 88200, 88200), 1);    /* the loop starts again on 88200 */
    CHECK_EQ(first_sound(buf + 2 * 2 * 88200, 88200), 1);
}

/* the sound does not depend on how the audio blocks are cut */
static void test_blocks(void)
{
    model_init();
    phrase_set(0, 0, STEP_NOTE, 48);
    phrase_set(0, 0, STEP_INSTR, 0);
    phrase_set(0, 3, STEP_NOTE, 55);                 /* legato */
    phrase_set(0, 5, STEP_VOL, 0x40);                /* volume alone */
    phrase_set(0, 6, STEP_NOTE, 60);
    phrase_set(0, 6, STEP_INSTR, 0);                 /* retrigger: fade */
    phrase_set(0, 6, STEP_VOL, 0xC0);
    phrase_set(0, 9, STEP_NOTE, NOTE_OFF);
    phrase_set(0, 12, STEP_NOTE, 72);
    phrase_set(0, 12, STEP_INSTR, 0);
    render(buf, MAXF, 256);
    render(buf2, MAXF, 0);
    CHECK(!memcmp(buf, buf2, sizeof buf));
    render(buf2, MAXF, 1);
    CHECK(!memcmp(buf, buf2, sizeof buf));
    render(buf2, MAXF, 5513);
    CHECK(!memcmp(buf, buf2, sizeof buf));
    CHECK(first_sound(buf, MAXF) == 1);
}

/* a new note on a sounding voice: no click, and it starts within 96 frames (2.2 ms) */
static void test_fade(void)
{
    static int32_t lr[2 * 4000];
    voice_t v;
    int i, n, worst = 0, ok_inc = 0;
    model_init();
    phrase_set(0, 0, STEP_NOTE, 57);
    phrase_set(0, 0, STEP_INSTR, 0);
    phrase_set(0, 1, STEP_NOTE, 81);                 /* two octaves up, on step 1 */
    phrase_set(0, 1, STEP_INSTR, 0);
    render(buf, 20000, 256);
    for (i = 1; i < 20000; i++) {
        int d = abs(buf[2 * i] - buf[2 * (i - 1)]);
        if (d > worst)
            worst = d;
    }
    /* steepest slope of a +-16383 sine at 1760 Hz: 2 pi 1760 / 44100 * 16383 = 4108 */
    CHECK(worst <= 4200);
    printf("  fade: largest step between two samples %d\n", worst);

    for (n = 0; n < 16; n++) {                       /* the new note, wherever the sub-block is */
        voice_reset(&v);
        voice_note(&v, 57, 0xFF);
        memset(lr, 0, sizeof lr);
        voice_render(&v, lr, 1000 + n);
        voice_note(&v, 69, 0xFF);
        for (i = 0; i < 200 && v.inc != note_inc(69, 0); i++)
            voice_render(&v, lr, 1);
        if (i - 1 <= 96)                             /* i - 1 frames of fade, then the note */
            ok_inc++;
    }
    CHECK_EQ(ok_inc, 16);
    voice_off(&v);
    memset(lr, 0, sizeof lr);
    voice_render(&v, lr, 4000);
    CHECK(!voice_active(&v));
    CHECK_EQ(lr[2 * 3999], 0);
}

static void test_stop(void)
{
    int i, quiet = 1;
    one_note(0);
    render(buf, 10000, 256);
    CHECK_EQ(seq_playing, 1);
    CHECK_EQ(seq_row[0], 1);                         /* step 1 starts at 5512 */
    seq_cmd_stop();
    memset(buf, 0, 2 * 10000 * sizeof *buf);
    seq_render(buf, 10000);
    CHECK_EQ(seq_playing, 0);
    for (i = 4096 + 32; i < 10000; i++)
        if (buf[2 * i])
            quiet = 0;
    CHECK(quiet);
    CHECK(buf[2 * 100] != 0);                        /* it releases, it does not cut */
}

static void test_cmdq(void)
{
    uint32_t c, i;
    int full = 0;
    cmdq_init();
    CHECK_EQ(cmdq_pop(&c), 0);
    for (i = 0; i < CMDQ_N; i++)
        CHECK_EQ(cmdq_push(100 + i), 0);
    CHECK_EQ(cmdq_push(999), -1);
    for (i = 0; i < CMDQ_N; i++) {
        CHECK_EQ(cmdq_pop(&c), 1);
        if (c != 100 + i)
            full = 1;
    }
    CHECK(!full);
    CHECK_EQ(cmdq_pop(&c), 0);
    CHECK_EQ(seq_cmd_play_phrase(TRK_TRACKS, 0), -1);
    CHECK_EQ(seq_cmd_play_phrase(0, TRK_PHRASES), -1);
}

TEST_MAIN(RUN(test_clock); RUN(test_trigger); RUN(test_loop_and_off); RUN(test_blocks); RUN(test_fade);
          RUN(test_stop); RUN(test_cmdq);)
