// SPDX-License-Identifier: GPL-3.0-only
/* The FX command codes (spec §6), in the spec's order. Saved projects store these numbers: new
 * codes only ever go at the end. 0 is no command. What each does comes with P3. */
#pragma once

enum {
    FX_NONE,
    FX_ARP, FX_ARC, FX_PSL, FX_PBN, FX_VIB, FX_PIT, FX_FIN,             /* pitch */
    FX_DEL, FX_KIL, FX_RET, FX_CHA, FX_NTH, FX_RND, FX_HOP,             /* time and chance */
    FX_TBL, FX_TIC, FX_THO, FX_GRV, FX_TPO,                             /* structure */
    FX_VOL, FX_CUT, FX_RES, FX_FLT, FX_PAN, FX_DRV,                     /* sound */
    FX_SCH, FX_SDL, FX_SRV,                                             /* sends */
    FX_MDT, FX_MDF, FX_MRS, FX_MCR, FX_MTP,                             /* global */
    FX_COUNT
};

/* the 3-letter name of a code ("---" for none, "???" for a code this build does not know) */
const char *fx_name(int code);
