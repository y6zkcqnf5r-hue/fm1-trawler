// SPDX-License-Identifier: GPL-3.0-only
#include "ui/gfx.h"
#include "app/plat.h"
#include "font.h"

/* one glyph at the largest scale, sent with a single lcd_blit */
static uint16_t glyph[FONT_W * GFX_SCALE_MAX * FONT_H * GFX_SCALE_MAX];

/* the panel takes RGB565 with its bytes swapped (as lcd_fill does it) */
static uint16_t swap(uint16_t c) { return (uint16_t)((c >> 8) | (c << 8)); }

static int clamp_scale(int scale)
{
    if (scale < 1)
        return 1;
    if (scale > GFX_SCALE_MAX)
        return GFX_SCALE_MAX;
    return scale;
}

void gfx_text(int x, int y, const char *s, int scale, uint16_t fg, uint16_t bg)
{
    uint16_t f = swap(fg), b = swap(bg);
    int w, h, r, c, k, i;
    scale = clamp_scale(scale);
    w = FONT_W * scale;
    h = FONT_H * scale;
    for (; *s; s++, x += w) {
        int ch = (unsigned char)*s;
        const uint8_t *rows;
        if (ch < FONT_FIRST || ch > FONT_LAST)
            ch = '?';
        rows = FONT[ch - FONT_FIRST];
        for (r = 0; r < h; r++) {
            uint8_t bits = rows[r / scale];             /* 32-bit division, scale >= 1 */
            uint16_t *px = &glyph[r * w];
            for (c = 0, i = 0; c < FONT_W; c++) {
                uint16_t v = bits & (0x80u >> c) ? f : b;
                for (k = 0; k < scale; k++)
                    px[i++] = v;
            }
        }
        lcd_blit(x, y, w, h, glyph);
    }
}

int gfx_text_w(const char *s, int scale)
{
    int n = 0;
    while (s[n])
        n++;
    return n * FONT_W * clamp_scale(scale);
}

int gfx_text_h(int scale) { return FONT_H * clamp_scale(scale); }
