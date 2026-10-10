// SPDX-License-Identifier: GPL-3.0-only
#include "tracker/fx.h"

static const char NAMES[FX_COUNT][4] = {
    "---",
    "ARP", "ARC", "PSL", "PBN", "VIB", "PIT", "FIN",
    "DEL", "KIL", "RET", "CHA", "NTH", "RND", "HOP",
    "TBL", "TIC", "THO", "GRV", "TPO",
    "VOL", "CUT", "RES", "FLT", "PAN", "DRV",
    "SCH", "SDL", "SRV",
    "MDT", "MDF", "MRS", "MCR", "MTP",
};

const char *fx_name(int code) { return code >= 0 && code < FX_COUNT ? NAMES[code] : "???"; }
