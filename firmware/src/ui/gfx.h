// SPDX-License-Identifier: GPL-3.0-only
/* Drawing on the 240x240 RGB565 screen, over plat.h's lcd_fill / lcd_blit. Provisional (P0-04):
 * text only, in the one generated font (build/gen/font.h, Terminus 6x12). P1-04 adds the rest. */
#pragma once
#include <stdint.h>

#define GFX_SCALE_MAX 4

/* s at (x, y), each font pixel drawn as scale x scale (1..GFX_SCALE_MAX), fg on bg; a character
 * outside the font shows as '?' */
void gfx_text(int x, int y, const char *s, int scale, uint16_t fg, uint16_t bg);
int gfx_text_w(const char *s, int scale);   /* width in pixels */
int gfx_text_h(int scale);                  /* height in pixels */
