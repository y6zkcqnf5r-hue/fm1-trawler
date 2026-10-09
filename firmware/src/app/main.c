// SPDX-License-Identifier: GPL-3.0-only
/* Placeholder app, enough to drive the host simulator (P0-03): it shows the held keys and buttons
 * as rectangles, lights their LEDs, and plays silence. P0-04 replaces it with the "hello" app. */
#include "app/plat.h"

#define C_BG 0x10A2u                    /* RGB565 */
#define C_OFF_WHITE 0x8410u
#define C_OFF_BLACK 0x4208u
#define C_OFF_BTN 0x31A6u
#define C_ON 0xFFE0u

#define KEY_W 15                        /* 16 white keys across 240 px */

static uint32_t shown_keys, shown_buttons;
static int drawn;

static int is_black(int k)
{
    int s = (k + 5) % 12;               /* key 0 is F3 */
    return s == 1 || s == 3 || s == 6 || s == 8 || s == 10;
}

static void draw(uint32_t buttons, uint32_t keys)
{
    int k, i, white = 0;
    for (i = 0; i < B_COUNT; i++)
        lcd_fill(i * 17 + 1, 20, 15, 15, buttons >> i & 1u ? C_ON : C_OFF_BTN);
    for (k = 0; k < NKEYS; k++) {
        uint16_t on = keys >> k & 1u ? C_ON : 0;
        if (is_black(k)) {
            lcd_fill(white * KEY_W - 5, 120, 10, 35, on ? on : C_OFF_BLACK);
        } else {
            lcd_fill(white * KEY_W, 160, KEY_W - 1, 50, on ? on : C_OFF_WHITE);
            white++;
        }
    }
}

void app_init(void)
{
    lcd_fill(0, 0, 240, 240, C_BG);
    drawn = 0;
}

void app_frame(void)
{
    uint32_t buttons = plat_buttons(), keys = plat_keys();
    if (!drawn || buttons != shown_buttons || keys != shown_keys) {
        draw(buttons, keys);
        shown_buttons = buttons;
        shown_keys = keys;
        drawn = 1;
    }
    plat_leds(buttons, keys, 0);
}

void app_audio(int32_t *lr, int n)
{
    int i;
    for (i = 0; i < 2 * n; i++)
        lr[i] = 0;
}
