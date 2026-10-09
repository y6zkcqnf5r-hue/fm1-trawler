# 05 — Architecture technique

Le modèle vient de FoMni et X0X (Charles Vestal, « niveau A »), avec des apports de ChoralRoot, Felucca et Hortator (voir `02_ECOSYSTEME.md` §3).

## 1. Les trois cibles

Le même code de `firmware/src/` se compile pour trois cibles, qui ne diffèrent que par l'implémentation de `plat.h` :

| Cible | Implémentation de `plat.h` | Compilateur | Rôle |
| --- | --- | --- | --- |
| **Hôte** | `host/trawler_host.c` | `cc` (clang/gcc) | Simulateur scripté : tests, WAV, PNG |
| **Web** | `web/emu/trawler_web.c`, qui inclut le fichier hôte | `emcc` | Le FM-1 virtuel dans le navigateur |
| **Appareil** (P5) | `firmware/src/app/plat_fm1.c`, au-dessus de `firmware/hal/` (repris de Felucca) | clang JieLi pi32v2 (Docker) | Le `.fwsc` |

```
            ┌──────────── firmware/src (commun, entiers seulement) ────────────┐
            │  dsp/  moteurs, filtres, effets    tracker/  modèle, séq., FX,     │
            │  ui/   vues, gfx, police           sérialisation                   │
            │  app/  main.c : app_init(), app_audio(), app_frame()               │
            └───────────────────────────────┬───────────────────────────────────┘
                                            │  plat.h  (≈20 fonctions)
            ┌───────────────────┬───────────┴───────────┬───────────────────────┐
     host/trawler_host.c   web/emu/trawler_web.c   app/plat_fm1.c + hal/
     (scénarios, WAV/PNG)  (wasm + AudioWorklet)   (FM-1 réel, phase 5)
```

## 2. L'arborescence cible

```
FM1/                               (ce dossier = la racine du repo)
├─ CLAUDE.md  README.md  LICENSE (GPL-3.0)  LICENSING.md  Makefile  .gitignore
├─ docs/                           PRD, spec, archi, backlog, décisions
├─ firmware/
│  ├─ src/
│  │  ├─ app/   plat.h  main.c  config.h (limites, budgets)  plat_fm1.c (P5)
│  │  ├─ tracker/ model.h model.c  seq.c  fx.c fx.h  table.c  groove.c  edit.c (copier/coller/annuler)  serial.c
│  │  ├─ dsp/   fixed.h (Q15/Q12, mul, sat)  osc.c  svf.c  env.c  lfo.c
│  │  │         eng_wav.c  eng_fm4.c  eng_drm.c  voice.c  mixer.c  fx_chorus.c fx_delay.c fx_reverb.c fx_master.c
│  │  └─ ui/    gfx.c (primitives RGB565 + texte)  font.c  ui.c (dispatch)  view_song.c view_chain.c view_phrase.c
│  │            view_instr.c view_table.c view_groove.c view_mixer.c view_project.c  help.c  input.c (combinaisons, pavé hexa)
│  ├─ hal/  loader/  app.ld  crt0.S          (P5 : repris de Felucca)
├─ host/   trawler_host.c  scenario.c  wav.c  png.c
├─ web/
│  ├─ emu/  trawler_web.c  worklet.js  index.html  panel.js (SVG + zones de clic)  keymap.js  storage.js  export.js
│  └─ demos/ synthwave.trw  lofi.trw
├─ tests/
│  ├─ unit/       test_fixed.c test_seq.c test_serial.c test_edit.c ...
│  ├─ scenarios/  *.trs   (press/turn/key/wait/wav/shot/expect)
│  ├─ golden/     hashes SHA-256 des WAV et PNG de référence
│  └─ host/emu_test.mjs  (le wasm et le natif rendent les mêmes octets)
├─ tools/  gen_font.py  gen_tables.py  size_report.py  lint.sh  make_demo.py
└─ vendor-ref/   (gitignore) clones en lecture seule
```

## 3. `plat.h`, la frontière (proposition)

```c
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <stdint.h>

/* --- temps --- */
uint32_t plat_ms(void);                     /* ms depuis le démarrage */

/* --- entrées (lues dans app_frame, ~60 Hz) --- */
enum { B_OCTDN, B_OCTUP, B_FX, B_SEL, B_ENV, B_LFO, B_EDIT, B_GLO,
       B_HOME, B_SAVE, B_ARP, B_SEQ, B_PLAY, B_REC, B_COUNT };       /* 14 */
enum { EN_SELECT, EN_ALGO, EN_K1, EN_K2, EN_K3, EN_K4, EN_PRESETS, EN_COUNT }; /* 7 */
uint32_t plat_buttons(void);                /* 1 bit par bouton, 1 = enfoncé */
uint32_t plat_keys(void);                   /* 27 bits, bit 0 = F3 */
int      plat_enc(int role);                /* crans accumulés depuis le dernier appel (signé) */
uint16_t plat_master(void);                 /* 0..4096 (Q12) */

/* --- sorties --- */
void plat_leds(uint32_t buttons, uint32_t keys, uint32_t keys_dim);
void lcd_fill(int x, int y, int w, int h, uint16_t rgb565);
void lcd_blit(int x, int y, int w, int h, const uint16_t *px);

/* --- MIDI (paquets USB-MIDI de 4 octets) --- */
int  plat_midi_in(uint32_t *pkt);           /* 1 si un paquet a été lu */
void plat_midi_out(uint32_t pkt);

/* --- stockage par objets (pas de flash brute) --- */
int  plat_store_load(int slot, uint8_t *buf, int max);   /* octets lus, ou -1 */
int  plat_store_save(int slot, const uint8_t *buf, int len); /* 0 = OK ; l'appareil n'écrit qu'à l'arrêt */

/* --- diagnostics --- */
int  plat_cpu_pct(void);                    /* charge audio estimée, ou mesurée sur l'appareil */
int  plat_xruns(void);

/* --- grosse mémoire --- */
#ifdef APP_DEVICE
#  define APP_POOL __attribute__((section(".pool")))
#else
#  define APP_POOL
#endif
```

L'app, de son côté, fournit trois fonctions à la plateforme :

```c
void app_init(void);
void app_audio(int32_t *lr, int n);   /* n = 256 frames entrelacées G/D, Q15 dans un int32 ; appelée depuis le callback audio */
void app_frame(void);                 /* boucle principale : entrées, interface, dessin, ~60 Hz */
```

**Partage entre l'audio et l'interface :** l'interface envoie ses changements au séquenceur par une file de commandes sans verrou, à un seul écrivain (`tracker/cmdq.c`). L'audio publie sa position de lecture dans des `volatile uint8_t`. Le modèle de projet est écrit par l'interface et lu par l'audio, ce qui est sûr octet par octet.

## 4. Les builds

```make
CFLAGS_COMMON = -std=gnu99 -O2 -Wall -Wextra -Werror=double-promotion -ffp-contract=off -fwrapv \
                -Ifirmware/src -Ibuild/gen

host:  cc $(CFLAGS_COMMON) -DAPP_HOST host/trawler_host.c <sources> -o build/host/trawler
web:   docker run --rm -v $(PWD):/src emscripten/emsdk:<VERSION_FIGÉE> \
         emcc $(CFLAGS_COMMON) -DAPP_HOST -DAPP_WEB --no-entry -sSTANDALONE_WASM -sFILESYSTEM=0 \
         -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=33554432 -o build/emu/trawler.wasm web/emu/trawler_web.c <sources>
```

- **Sans glue Emscripten :** le worklet instancie le `.wasm` avec un faux WASI (un `Proxy` qui renvoie 0), comme X0X.
- **Exports :** chaque export est marqué `__attribute__((used, visibility("default")))` :

  | Groupe | Exports |
  | --- | --- |
  | Démarrage | `web_boot()` |
  | Audio | `web_render(n)`, `web_out_l()`, `web_out_r()` |
  | Entrées | `web_buttons(mask)`, `web_keys(mask)`, `web_enc(role, n)`, `web_master(v)`, `web_midi(st, d1, d2)` |
  | Écran et LED | `web_fb()`, `web_blits()`, `web_lit_buttons()`, `web_lit_keys()`, `web_dim_keys()` |
  | Stockage | `web_store_ptr(slot)`, `web_store_len(slot)`, `web_store_set_len(slot, n)`, `web_store_writes()` |
  | Fichiers | `web_export_wav(...)` (P4) |
  | Diagnostics | `web_cpu()`, `web_now()` |

- **Lint (`tools/lint.sh`) :**
  - interdit `float`, `double`, `malloc`, `printf`, `#include <math.h>`, `/ … int64` (heuristique), `APP_WEB` et `APP_HOST` dans `firmware/src` ;
  - exige l'en-tête SPDX ;
  - signale les constantes 0xFFC…

## 5. Le protocole entre la page et le worklet

On reprend celui de X0X et FoMni :

- **De la page au worklet :**
  - `{load, wasm, store}` ;
  - `{buttons, mask}` et `{keys, mask}` : chaque changement passe par une file et reste maintenu au moins **24 ms** en temps de l'appareil, pour franchir l'anti-rebond ;
  - `{enc, role, n}`, `{master, v}`, `{midi, data}`, `{visible, bool}`.
- **Du worklet à la page :**
  - `{ready}` ;
  - puis environ 30 fois par seconde (4 si l'onglet est caché) : `{frame, fb?, buttons, keys, dim, store?, cpu}`. Le tampon `fb` n'est transféré (`Uint16Array`) que si le compteur de blits a changé.

**Horloge :** `web_render(128)` fait avancer le temps de l'appareil. Il appelle `app_audio` par blocs de 256 frames, comme sur l'appareil, en remplissant un anneau, et `app_frame` toutes les 16 ms.

**Audio :**
- La page crée `new AudioContext({sampleRate: 44100})`.
- Si le navigateur impose une autre fréquence, le worklet rééchantillonne de façon linéaire (comme Salt) au lieu de jouer faux.
- Rien ne joue avant un clic : c'est la règle des navigateurs. D'ici là, une instance muette affiche déjà l'écran (comme ChoralRoot).

**Écran :** un canvas de 240×240, affiché en ×2 avec `image-rendering: pixelated`. Le RGB565 a ses octets inversés ; une table de correspondance le convertit en `ImageData`.

**Panneau :**
- Un dessin SVG du FM-1. Les zones de clic sont des `div` HTML transparents posés dessus, parce que le tactile sur du SVG est peu fiable sous iOS.
- Capture du pointeur et multi-touch. Maj-clic ou clic droit verrouille un bouton enfoncé, ce qui permet de maintenir SEL.
- Les potards se tournent en glissant ou à la molette.
- **Le dessin ne copie pas les photos de M-VAVE :** c'est une silhouette maison, aux proportions du FM-1.

**Stockage :** IndexedDB, avec repli sur localStorage, chaque accès dans un `try/catch`.

## 6. Les tests

| Niveau | Outil | Ce qu'on vérifie |
| --- | --- | --- |
| Unitaires | `tests/unit/*.c` (mini-framework maison) | Point fixe, horloge Q8 sans dérive sur 10 min, groove, chaque commande FX, aller-retour `.trw`, annuler/rétablir |
| Scénarios | `build/host/trawler tests/scenarios/x.trs out/` | Parcours d'interface : `press SEQ`, `turn PRESETS 3`, `key 12`, `wait 500`, `shot`, `wav 2000`, `expect cell 0 note C-4` |
| Golden | `tests/golden/*.sha256` | Le WAV et le PNG d'un scénario ne changent pas sans qu'on le veuille (`make golden` pour les accepter) |
| Parité | `node tests/host/emu_test.mjs` | Le wasm et le natif rendent les mêmes octets sur 10 s de démo |
| Taille | `tools/size_report.py` | RAM, pool et taille du projet sous les budgets ; à terme, la taille du code compilé pour l'appareil |
| CI | GitHub Actions | `make lint test web`, puis déploiement de `build/emu` sur GitHub Pages |

## 7. Le budget CPU sans appareil (modèle de coût)

Tant qu'on n'a pas de FM-1, on ne peut pas mesurer le CPU réel. On applique donc ces garde-fous :

1. **Un compteur d'opérations, rudimentaire :** chaque moteur déclare un coût `cost_per_frame` en multiplications-accumulations (MAC) dans son descripteur. `voice.c` additionne le coût des voix actives et des effets, et `plat_cpu_pct()` le rapporte à un budget de référence.
2. **Le budget de référence :** on le fixe d'après les moteurs connus de Felucca, qui tient 8 voix plus ses effets. On prend une marge et on vise **≤ 50 %** avec 8 pistes actives et tous les effets.
3. **Dans le navigateur :** une jauge « CPU estimé » affiche cette valeur. Elle passe au rouge au-dessus de 50 %.
4. **En P5 :** on mesure avec `fm1_ticks()` autour de `app_audio` et on recalibre la référence.

C'est une estimation : elle sert à éviter les mauvaises surprises, pas à garantir l'absence de craquements.

## 8. Le portage sur l'appareil (P5, résumé)

1. Copier dans `firmware/` les éléments de Felucca : `hal/`, `loader/`, `app.ld`, `crt0.S`, `libc.c`, `tools/build.py`, `fm1pkg_make.py`, `get_toolchain.sh`. En-têtes GPL conservés, crédits dans `LICENSING.md`.
2. Écrire `plat_fm1.c` : les entrées par `fm1_input_edges`/`fm1_enc_take`, l'audio par le callback `fm1_alnk0_irq` qui appelle `app_audio`, et le stockage dans la zone de données (0x97000–0xE0000), écrit seulement à l'arrêt.
3. Prendre l'identité **FM-1_95N** (à confirmer), et remplacer le VID/PID USB de test.
4. Reprendre le mode secours (OCT− à l'allumage) et le garde-fou des deux crashs au démarrage, comme Dinghy.
5. Publier une release GitHub avec le `.fwsc`, le texte de la licence Apache des fichiers JieLi et le source, puis demander l'inscription sur le hub.
