// SPDX-License-Identifier: GPL-3.0-only
/* The scenario runner: the whole app on a simulated FM-1, driven by a .trs script. Audio goes to
 * WAV files, the screen to PNGs. Language from FoMni's host/omni_host.c (Charles Vestal).
 *
 *   trawler SCRIPT [OUTDIR]       exit 0 = all expectations met, 1 = some failed, 2 = script error
 *
 * One command per line, # at a word start begins a comment. Files are written in OUTDIR (default ".").
 *   wait MS                       run the device for MS ms
 *   press BTN | release BTN       hold / let go, then 2 ms
 *   tap BTN                       held 40 ms, then 40 ms (past one app_frame and the 24 ms debounce)
 *   key K [down|up]               no argument: held 60 ms, then 30 ms
 *   turn ENC N [MS]               N signed detents, one every MS ms (default 80)
 *   master N                      MASTER pot, 0..4096
 *   midi S D1 D2                  an incoming USB-MIDI message (hex bytes)
 *   wav FILE [MS]                 start recording; with MS, record MS ms then stop
 *   wavstop                       stop recording (also done at the end)
 *   shot FILE.png                 save the screen
 *   leds                          print the lit buttons and keys
 *   peakreset                     restart the output peak
 *   expect WHAT [ARGS] VAL        VAL is N, 0xN, <N or >N; WHAT is a probe below
 *   echo TEXT                     print TEXT
 *
 *   BTN  OCT- OCT+ FX SEL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC
 *   ENC  SELECT ALGO K1 K2 K3 K4 PRESETS (or PRESET)
 *   K    0..26 (0 = F3), w0..w15 (white keys), b0..b10 (black keys), or a note F3..G5 (C4, C#4, C-4)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host.h"
#include "io.h"

static const char *const BTN_N[B_COUNT] = {"OCT-", "OCT+", "FX", "SEL", "ENV", "LFO", "EDIT",
                                           "GLO", "HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC"};
static const char *const ENC_N[EN_COUNT] = {"SELECT", "ALGO", "K1", "K2", "K3", "K4", "PRESETS"};

static int lineno;

static int bad(const char *what, const char *s)
{
    printf("line %d: %s %s\n", lineno, what, s);
    exit(2);
}

static int btn_of(const char *s)
{
    int i;
    for (i = 0; i < B_COUNT; i++)
        if (!strcmp(BTN_N[i], s))
            return i;
    return bad("no button", s);
}

static int enc_of(const char *s)
{
    int i;
    if (!strcmp(s, "PRESET"))
        return EN_PRESETS;
    for (i = 0; i < EN_COUNT; i++)
        if (!strcmp(ENC_N[i], s))
            return i;
    return bad("no encoder", s);
}

/* a whole decimal or 0x number, or a script error */
static long num(const char *s)
{
    char *end;
    long v;
    if (!s || !*s)
        return bad("missing number", "");
    v = strtol(s, &end, 0);
    if (*end)
        bad("not a number:", s);
    return v;
}

static int key_of(const char *s)
{
    static const int semi[7] = {9, 11, 0, 2, 4, 5, 7};   /* A B C D E F G */
    long k;
    if (!*s)
        return bad("missing key", "");
    if ((s[0] == 'w' || s[0] == 'b') && s[1] >= '0' && s[1] <= '9') {
        k = num(s + 1);
        if (s[0] == 'w' && k >= 0 && k < 16)
            return host_white[k];
        if (s[0] == 'b' && k >= 0 && k < 11)
            return host_black[k];
        return bad("no key", s);
    }
    if (s[0] >= 'A' && s[0] <= 'G') {
        const char *p = s + 1;
        int n = semi[s[0] - 'A'];
        if (*p == '#') {
            n++;
            p++;
        } else if (*p == '-')
            p++;
        if (p[0] < '0' || p[0] > '9' || p[1])
            return bad("no key", s);
        k = (p[0] - '0') * 12 + n - (3 * 12 + 5);     /* F3 = 0 */
    } else
        k = num(s);
    if (k < 0 || k >= NKEYS)
        return bad("no key", s);
    return (int)k;
}

/* ------------------------------------------------------------- probes --- */
/* expect WHAT [ARGS] VAL. Later stories add the app's state here (cell, view, ...). */
static long p_ms(char **a) { (void)a; return (long)host.now_ms; }
static long p_buttons(char **a) { (void)a; return (long)host.buttons; }
static long p_keys(char **a) { (void)a; return (long)host.keys; }
static long p_enc(char **a) { return host.enc[enc_of(a[0])]; }
static long p_lit_buttons(char **a) { (void)a; return (long)host.lit_buttons; }
static long p_lit_keys(char **a) { (void)a; return (long)host.lit_keys; }
static long p_dim_keys(char **a) { (void)a; return (long)host.dim_keys; }
static long p_play_red(char **a) { (void)a; return host.play_red; }
static long p_blits(char **a) { (void)a; return (long)host.blits; }
static long p_wav_frames(char **a) { (void)a; return (long)wav_frames(); }
static long p_peak(char **a) { (void)a; return host.peak; }
static long p_midi_out(char **a) { (void)a; return (long)host.midi_out; }
static long p_store_writes(char **a) { (void)a; return (long)host.store_writes; }
static long p_pixel(char **a)           /* RGB565 as the app drew it */
{
    long x = num(a[0]), y = num(a[1]);
    uint16_t p;
    if (x < 0 || x >= LCD_W || y < 0 || y >= LCD_H)
        bad("pixel outside the screen:", a[0]);
    p = host.fb[y * LCD_W + x];
    return (uint16_t)((p >> 8) | (p << 8));
}

static const struct probe {
    const char *name;
    int nargs;
    long (*get)(char **args);
} PROBES[] = {
    {"ms", 0, p_ms},
    {"buttons", 0, p_buttons},
    {"keys", 0, p_keys},
    {"enc", 1, p_enc},
    {"lit_buttons", 0, p_lit_buttons},
    {"lit_keys", 0, p_lit_keys},
    {"dim_keys", 0, p_dim_keys},
    {"play_red", 0, p_play_red},
    {"blits", 0, p_blits},
    {"pixel", 2, p_pixel},
    {"wav_frames", 0, p_wav_frames},
    {"peak", 0, p_peak},
    {"midi_out", 0, p_midi_out},
    {"store_writes", 0, p_store_writes},
};

/* 0 = met, 1 = failed */
static int expect(char **tok, int ntok)
{
    const struct probe *p = 0;
    const char *val;
    long got, lim;
    int i, ok;
    for (i = 0; i < (int)(sizeof PROBES / sizeof *PROBES); i++)
        if (!strcmp(PROBES[i].name, tok[1]))
            p = &PROBES[i];
    if (!p)
        bad("unknown expect", tok[1]);
    if (ntok != 3 + p->nargs)
        bad("wrong argument count for expect", tok[1]);
    got = p->get(tok + 2);
    val = tok[2 + p->nargs];
    if (val[0] == '<' || val[0] == '>') {
        lim = num(val + 1);
        ok = val[0] == '<' ? got < lim : got > lim;
    } else
        ok = got == num(val);
    printf("%s line %d: expect %s", ok ? "  ok" : "FAIL", lineno, tok[1]);
    for (i = 0; i < p->nargs; i++)
        printf(" %s", tok[2 + i]);
    printf(" %s, got %ld (0x%lX)\n", val, got, (unsigned long)got);
    return !ok;
}

/* -------------------------------------------------------------- main --- */
static void leds(void)
{
    int i;
    printf("  leds: buttons");
    for (i = 0; i < B_COUNT; i++)
        if (host.lit_buttons >> i & 1u)
            printf(" %s", BTN_N[i]);
    if (host.play_red)
        printf(" (PLAY red)");
    printf(" | white ");
    for (i = 0; i < 16; i++)
        putchar(host.lit_keys >> host_white[i] & 1u ? '#' : host.dim_keys >> host_white[i] & 1u ? '+' : '.');
    printf(" black ");
    for (i = 0; i < 11; i++)
        putchar(host.lit_keys >> host_black[i] & 1u ? '#' : host.dim_keys >> host_black[i] & 1u ? '+' : '.');
    putchar('\n');
}

int main(int argc, char **argv)
{
    char line[512], path[1024], *tok[8];
    const char *dir = argc > 2 ? argv[2] : ".";
    int ntok, fails = 0;
    FILE *sc;
    if (argc < 2) {
        fprintf(stderr, "usage: trawler SCRIPT [OUTDIR]\n");
        return 2;
    }
    sc = fopen(argv[1], "r");
    if (!sc) {
        perror(argv[1]);
        return 2;
    }
    host_audio_sink = wav_write;
    host_boot();
    while (fgets(line, sizeof line, sc)) {
        char *c, *cmd;
        lineno++;
        for (c = line; *c; c++)         /* a comment starts with a # at a word start (not C#4) */
            if (*c == '#' && (c == line || c[-1] == ' ' || c[-1] == '\t')) {
                *c = 0;
                break;
            }
        c = line;
        for (ntok = 0; ntok < 8 && (tok[ntok] = strtok(c, " \t\r\n")); ntok++)
            c = 0;
        if (!ntok)
            continue;
        cmd = tok[0];
        if (!strcmp(cmd, "wait"))
            host_run_ms((uint32_t)num(tok[1]));
        else if (!strcmp(cmd, "press") || !strcmp(cmd, "release")) {
            uint32_t bit = 1u << btn_of(ntok > 1 ? tok[1] : "");
            host.buttons = cmd[0] == 'p' ? host.buttons | bit : host.buttons & ~bit;
            host_run_ms(2);
        } else if (!strcmp(cmd, "tap")) {
            uint32_t bit = 1u << btn_of(ntok > 1 ? tok[1] : "");
            host.buttons |= bit;
            host_run_ms(40);
            host.buttons &= ~bit;
            host_run_ms(40);
        } else if (!strcmp(cmd, "key")) {
            uint32_t bit = 1u << key_of(ntok > 1 ? tok[1] : "");
            if (ntok < 3) {
                host.keys |= bit;
                host_run_ms(60);
                host.keys &= ~bit;
                host_run_ms(30);
            } else if (!strcmp(tok[2], "down") || !strcmp(tok[2], "up")) {
                host.keys = tok[2][0] == 'd' ? host.keys | bit : host.keys & ~bit;
                host_run_ms(2);
            } else
                bad("key: down or up, not", tok[2]);
        } else if (!strcmp(cmd, "turn")) {
            int e = enc_of(ntok > 1 ? tok[1] : "");
            long n = num(ntok > 2 ? tok[2] : 0), gap = ntok > 3 ? num(tok[3]) : 80;
            int s = n > 0 ? 1 : -1;
            for (; n; n -= s) {
                host.enc[e] += s;
                host_run_ms((uint32_t)gap);
            }
        } else if (!strcmp(cmd, "master")) {
            long v = num(ntok > 1 ? tok[1] : 0);
            if (v < 0 || v > 4096)
                bad("master: 0..4096, not", tok[1]);
            host.master = (uint16_t)v;
        } else if (!strcmp(cmd, "midi")) {
            uint32_t s, d1, d2;
            if (ntok != 4)
                bad("midi: S D1 D2", "");
            s = (uint32_t)strtoul(tok[1], 0, 16) & 0xFF;
            d1 = (uint32_t)strtoul(tok[2], 0, 16) & 0x7F;
            d2 = (uint32_t)strtoul(tok[3], 0, 16) & 0x7F;
            host_midi_push((s >= 0xF0 ? 0x0Fu : s >> 4) | s << 8 | d1 << 16 | d2 << 24);
        } else if (!strcmp(cmd, "wav")) {
            if (ntok < 2)
                bad("wav: FILE [MS]", "");
            snprintf(path, sizeof path, "%s/%s", dir, tok[1]);
            if (wav_open(path))
                bad("cannot write", path);
            if (ntok > 2) {
                host_run_ms((uint32_t)num(tok[2]));
                wav_close();
            }
        } else if (!strcmp(cmd, "wavstop"))
            wav_close();
        else if (!strcmp(cmd, "shot")) {
            if (ntok < 2)
                bad("shot: FILE.png", "");
            snprintf(path, sizeof path, "%s/%s", dir, tok[1]);
            if (png_write(path, host.fb))
                bad("cannot write", path);
        } else if (!strcmp(cmd, "leds"))
            leds();
        else if (!strcmp(cmd, "peakreset"))
            host.peak = 0;
        else if (!strcmp(cmd, "expect")) {
            if (ntok < 3)
                bad("expect: WHAT [ARGS] VAL", "");
            fails += expect(tok, ntok);
        } else if (!strcmp(cmd, "echo")) {
            int i;
            for (i = 1; i < ntok; i++)
                printf(i > 1 ? " %s" : "%s", tok[i]);
            putchar('\n');
        } else
            bad("unknown command", cmd);
    }
    fclose(sc);
    wav_close();
    printf("host: %u ms simulated, %u blits, %u WAV frames%s\n", host.now_ms, host.blits, wav_frames(),
           fails ? ", EXPECTATIONS FAILED" : "");
    return fails ? 1 : 0;
}
