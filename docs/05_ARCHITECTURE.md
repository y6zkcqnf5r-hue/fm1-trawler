# 05 — Architecture technique

Le modèle vient de FoMni et X0X (Charles Vestal, « niveau A »), avec des apports de ChoralRoot, Felucca et Hortator (voir `02_ECOSYSTEME.md` §3).

## 1. Les trois cibles

Le même code de `firmware/src/` se compile pour trois cibles, qui ne diffèrent que par l'implémentation de `plat.h` :

| Cible | Implémentation de `plat.h` | Compilateur | Rôle |
| --- | --- | --- | --- |
| **Hôte** | `host/trawler_host.c` | `cc` (clang/gcc) | Simulateur scripté : tests, WAV, PNG |
| **Web** | `web/emu/trawler_web.c`, compilé avec le fichier hôte | `emcc` | Le FM-1 virtuel dans le navigateur |
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
├─ assets/fonts/                   ter-u12n.bdf, ter-u16n.bdf (Terminus 6×12 et 8×16, OFL ; faces 6×10 et 8×12 découpées, ADR-027)
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
│  ├─ emu/  trawler_web.c  worklet.js  index.html  panel.js (SVG + zones de clic)  keymap.js  storage.js (P2-08)  export.js (P4)
│  └─ demos/ synthwave.trw  lofi.trw
├─ tests/
│  ├─ unit/       test_fixed.c test_seq.c test_serial.c test_edit.c ...
│  ├─ scenarios/  *.trs   (press/turn/key/wait/wav/shot/expect)
│  ├─ golden/     <scénario>.sha256 : hashes SHA-256 des WAV et PNG de référence
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

**Partage entre l'audio et l'interface :** l'interface envoie ses changements au séquenceur par une file de commandes sans verrou, à un seul écrivain (`tracker/cmdq.c`, 16 commandes de 32 bits, vidée au début de chaque bloc audio ; `seq_cmd_play_phrase`, `seq_cmd_stop`). L'audio publie sa position de lecture dans des `volatile uint8_t`. Le modèle de projet est écrit par l'interface et lu par l'audio, ce qui est sûr octet par octet.

## 4. Les builds

```make
CFLAGS_COMMON = -std=gnu99 -O2 -Wall -Wextra -Werror=double-promotion -ffp-contract=off -fwrapv \
                -Ifirmware/src -Ibuild/gen

host:  cc $(CFLAGS_COMMON) -DAPP_HOST host/trawler_host.c <sources> -o build/host/trawler
web:   emcc $(CFLAGS_COMMON) -DAPP_HOST -DAPP_WEB -Ihost --no-entry -sSTANDALONE_WASM -sFILESYSTEM=0 \
         -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=33554432 -o build/emu/trawler.wasm \
         web/emu/trawler_web.c host/trawler_host.c <sources>
```

- **emcc :** celui de l'emsdk local (`~/emsdk`, version 6.0.11, ADR-011), ou celui de l'image `emscripten/emsdk:6.0.11` avec `DOCKER=1`. L'image tourne sous l'utilisateur courant (`-u`), avec le cache d'Emscripten dans `build/emcache` : les fichiers de `build/` ne deviennent pas ceux de root. Les deux chemins donnent le même `.wasm`, à l'octet près. Sans emcc, `make web` s'arrête avec un message qui dit quoi installer.
- `make web` copie aussi `index.html`, `worklet.js` et `keymap.js` dans `build/emu/` ; `make serve` sert ce dossier sur `http://127.0.0.1:8080`.

- **Sans glue Emscripten :** le worklet instancie le `.wasm` avec un faux WASI (un `Proxy` qui renvoie 0), comme X0X.
- **Exports :** chaque export est marqué `__attribute__((used, visibility("default")))` :

  | Groupe | Exports |
  | --- | --- |
  | Démarrage | `web_boot()` |
  | Audio | `web_render(n)` (n ≤ 1024), `web_out_l()`, `web_out_r()` (des `int16`, la page convertit en float), `web_avail()`, `web_run_ms(ms)` (instance muette et test) |
  | Entrées | `web_buttons(mask)`, `web_keys(mask)`, `web_enc(role, n)`, `web_master(v)`, `web_midi(st, d1, d2)` |
  | Écran et LED | `web_fb()`, `web_blits()`, `web_lit_buttons()`, `web_lit_keys()`, `web_dim_keys()`, `web_play_red()` |
  | Stockage | `web_slots()`, `web_store_max()`, `web_store_ptr(slot)`, `web_store_len(slot)`, `web_store_set_len(slot, n)`, `web_store_writes()` |
  | Fichiers | `web_export_wav(...)` (P4) |
  | Diagnostics | `web_cpu()`, `web_now()`, `web_peak()` |

- **Lint (`tools/lint.sh`) :**
  - interdit `float`, `double`, `malloc`, `printf`, `#include <math.h>`, `/ … int64` (heuristique), `APP_WEB` et `APP_HOST` dans `firmware/src` ;
  - exige l'en-tête SPDX ;
  - signale les constantes 0xFFC…

## 5. Le protocole entre la page et le worklet

On reprend celui de X0X et FoMni :

Chaque message porte un champ `type` (`{type: "keys", mask}`), comme dans X0X ; le détail est en tête de `web/emu/worklet.js`.

- **De la page au worklet :**
  - `{load, wasm, store, master}` ;
  - `{buttons, mask}` et `{keys, mask}` : une file par type ; chaque changement reste maintenu au moins **24 ms** en temps de l'appareil (`web_now()`), pour franchir l'anti-rebond. Un appui plus court que la page ne sait le dire arrive donc quand même à l'app ;
  - `{enc, role, n}`, `{master, value}`, `{midi, data: [st, d1, d2]}`, `{visible, value}`.
- **Du worklet à la page :**
  - `{ready, rate}`, ou `{error, message}` ;
  - puis environ 30 fois par seconde (4 si l'onglet est caché) : `{frame, fb?, buttons, keys, dim, red, store?, cpu, load?, now, frames, nz, peak}`. Le tampon `fb` n'est transféré (`Uint16Array`) que si le compteur de blits a changé. `now`, `frames`, `nz` (échantillons non nuls) et `peak` servent au diagnostic.

**Horloge :** `web_render(n)` fait avancer le temps de l'appareil. `host_audio_sink` remplit un anneau `int16` de 4096 frames dans `trawler_web.c` ; tant qu'il manque des frames, `web_render` appelle `host_run_ms(1)`, qui rend l'app par blocs de 256 frames, comme sur l'appareil, et appelle `app_frame` toutes les 16 ms. Aucune horloge réelle n'entre : le temps de l'appareil ne dépend que des frames consommées, il ne peut pas dériver (vérifié sur 10 min par `emu_test.mjs`). Il avance par sauts d'un bloc (< 6 ms).

**Audio :**
- La page crée `new AudioContext({sampleRate: 44100})`.
- Si le navigateur impose une autre fréquence, le worklet rééchantillonne de façon linéaire (comme Salt) au lieu de jouer faux. La phase est un entier (`acc += 44100` par frame de sortie, une frame source de plus à chaque `sampleRate`) : un quantum consomme exactement `floor((acc + 128·44100) / sampleRate)` frames, d'où un seul `web_render` par quantum et aucune dérive.
- Rien ne joue avant un clic : c'est la règle des navigateurs. D'ici là, une instance muette affiche déjà l'écran (comme ChoralRoot).

**Écran :** un canvas de 240×240, affiché en ×2 avec `image-rendering: pixelated`. Le RGB565 a ses octets inversés ; une table de 65 536 entrées le convertit en `ImageData`.

**Clavier (`keymap.js`) :** par `KeyboardEvent.code`, donc par position quelle que soit la disposition (spec §4.5). Tout ce qui est maintenu est relâché sur `blur` et quand l'onglet se cache. Toute la spec §4.5 ; trois légendes (QWERTY, AZERTY Mac, AZERTY PC), choisies par l'utilisateur, sinon par `navigator.keyboard.getLayoutMap()` (Chrome, Edge), sinon par la langue du navigateur.

**Panneau (`web/emu/panel.js`) :**
- Un dessin SVG du FM-1 (viewBox 1000×640, géométrie de X0X) : les 27 touches avec les légendes gravées des noires et les points entre les groupes, les 14 boutons, les 7 encodeurs, MASTER, et l'écran posé dans son cadre. Sur chaque contrôle, la touche d'ordinateur qui le joue (masquable).
- Les zones de clic sont des `div` HTML transparents posés dessus, parce que le tactile sur du SVG est peu fiable sous iOS.
- Capture du pointeur et multi-touch : une carte « qui tient quoi » par pointeur et par touche d'ordinateur, dont l'union part au worklet. **Clic droit (ou Ctrl-clic), ou appui long de 0,5 s au doigt, verrouille un bouton** ; un nouvel appui le libère. L'appui long ne verrouille que si rien d'autre n'a été touché pendant l'appui. Maj, qui est SEL, + clic = SEL + le contrôle (ADR-018).
- Les potards se tournent en glissant verticalement (7 px par cran) ou à la molette ; MASTER va de 0 à 4096.
- **Taille :** le panneau prend la largeur, mais tient toujours dans la hauteur de la fenêtre (au plus 1600 px de large). L'écran est net (`pixelated`) dès qu'il a un pixel réel par pixel du LCD, lissé en dessous. En portrait sur téléphone, la page propose de tourner l'appareil.
- **Le dessin ne copie pas les photos de M-VAVE :** c'est une silhouette maison, aux proportions du FM-1.

**Stockage :** IndexedDB, avec repli sur localStorage, chaque accès dans un `try/catch`.

## 6. Les tests

| Niveau | Outil | Ce qu'on vérifie |
| --- | --- | --- |
| Unitaires | `tests/unit/*.c` (mini-framework maison) | Point fixe, horloge Q8 sans dérive sur 10 min, groove, chaque commande FX, aller-retour `.trw`, annuler/rétablir |
| Scénarios | `build/host/trawler tests/scenarios/x.trs out/` | Parcours d'interface : `press SEQ`, `turn PRESETS 3`, `key 12`, `wait 500`, `shot`, `wav 2000`, `expect cell 0 note C-4` |
| Golden | `tests/golden/*.sha256` | Le WAV et le PNG d'un scénario ne changent pas sans qu'on le veuille (`make golden` pour les accepter) |
| Rendus de référence | `tests/unit/render.h`, `tests/golden/renders/<suite>.sha256` | Les tests unitaires d'un moteur écrivent ses WAV dans `build/renders/<suite>/` ; `make unit` vérifie leurs hashes, `make golden` les accepte |
| Parité | `make parity` (`node tests/host/emu_test.mjs`, dans `make test`) | Le wasm rejoue le journal de chaque scénario de `tests/scenarios/` (`trawler --events`) : mêmes WAV que les golden et que le natif, même écran à chaque capture ; l'horloge sur 10 min ; `worklet.js` lui-même sous des bouchons, à 44,1 et 48 kHz ; `keymap.js` |
| Taille | `make size` (`tools/size_report.py`, dans `make test`) | `firmware/src` compilé par clang en objets ELF 32 bits (`--target=thumbv7m-none-eabi -ffreestanding -nostdlibinc -DAPP_DEVICE`, ADR-020), lus par un petit lecteur ELF : RAM (`.data` + `.bss`), pool (`.pool`), code (indicatif tant qu'on n'a pas pi32v2) et projet (le symbole `proj`), face aux budgets de `app/config.h`. Échoue si un budget saute, ou si un objet appelle une division 64 bits ou un flottant logiciel. En P5, le même script lit l'ELF lié de l'appareil |
| CI | GitHub Actions (`.github/workflows/ci.yml`) | `make lint test web DOCKER=1` à chaque push et PR (natif compilé par gcc sous Ubuntu, rapport de taille par clang) ; sur `main`, déploiement de `build/emu` sur GitHub Pages : https://y6zkcqnf5r-hue.github.io/fm1-trawler/ |

### Le simulateur hôte et le format `.trs`

Le simulateur se découpe en quatre fichiers :

| Fichier | Rôle | Builds |
| --- | --- | --- |
| `host/trawler_host.c` | Implémente `plat.h` sur l'état `struct host` (`host/host.h`), et fournit l'horloge `host_run_ms()` | Natif et wasm : sans stdio ni fichier |
| `host/scenario.c` | Le lanceur de scénarios | Natif seulement |
| `host/wav.c` | Écrit les WAV | Natif seulement |
| `host/png.c` | Écrit les PNG | Natif seulement |

**L'horloge, en entiers :**
- chaque ms ajoute 44 100 à un accumulateur ;
- chaque fois qu'il atteint 256 000, l'app rend un bloc de 256 frames ;
- `app_frame()` tourne toutes les 16 ms.

La sortie est saturée à 16 bits (ADR-012), puis passe à `host_audio_sink` : le WAV en natif, l'anneau du worklet en web.

**La commande :** `build/host/trawler [--events FICHIER] SCRIPT.trs [OUTDIR]`. Avec `--events`, le lanceur écrit aussi ce que le script a fait à l'appareil, une ligne par action (`b MASK`, `k MASK`, `m V`, `e ROLE N`, `midi PKT`, `run MS`, `wav on FICHIER`, `wav off`, `shot FICHIER FNV`) : `emu_test.mjs` le rejoue sur le wasm, sans second parseur de `.trs`. Elle sort avec 0 si tout passe, 1 si un `expect` échoue, 2 si le script est faux. `make scenarios` joue chaque `tests/scenarios/*.trs` dans `build/scenarios/<nom>/`, puis `tests/host/check_outputs.py` valide les PNG et les WAV produits, et `tests/host/golden.py check` compare leurs hashes à `tests/golden/<nom>.sha256`.

**Les golden :**
- un fichier par scénario, au format de `shasum -a 256` (`<hex>  <fichier>`, trié par nom) ;
- chaque WAV et PNG produit doit y figurer avec le même hash, et chaque ligne doit avoir son fichier ;
- `make golden` rejoue les scénarios et réécrit les `.sha256`. On ne le lance qu'après avoir écouté le WAV et regardé le PNG, et le diff des `.sha256` part dans le commit.

**Les polices :** `tools/gen_font.py` convertit les BDF de `assets/fonts/` en `build/gen/font.h` (ASCII 32–126, une ligne par octet) : trois faces, 6×12, 6×10 et 8×12, les deux dernières découpées dans Terminus (ADR-027). `ui/gfx.c` dessine rectangles, lignes, cadres et texte, et `ui/gfx.h` porte les couleurs du thème. `make mockups` dessine la vue Phrase dans les deux faces (`docs/mockups/`, Q-02).

**Le format :** une commande par ligne. Un `#` en début de mot ouvre un commentaire (`C#4` n'en est pas un).

| Commande | Effet |
| --- | --- |
| `wait MS` | Fait tourner l'appareil MS ms |
| `press BTN` / `release BTN` | Maintient ou relâche le bouton, puis avance de 2 ms |
| `tap BTN` | Appui de 40 ms, puis 40 ms de pause |
| `key K [down\|up]` | Sans argument : appui de 60 ms, puis 30 ms de pause |
| `turn ENC N [MS]` | N crans signés, un tous les MS ms (80 par défaut) |
| `master N` | Potard MASTER, de 0 à 4096 |
| `midi S D1 D2` | Message USB-MIDI entrant (en hexa) |
| `wav FICHIER [MS]` | Commence l'enregistrement ; avec MS, enregistre MS ms puis s'arrête |
| `wavstop` | Arrête l'enregistrement (fait aussi en fin de script) |
| `shot FICHIER.png` | Capture l'écran |
| `leds` | Affiche les LED allumées |
| `peakreset` | Remet le pic de sortie à zéro |
| `blitsreset` | Remet à zéro le compteur d'appels à l'écran |
| `echo TEXTE` | Affiche une trace |
| `expect QUOI [ARGS] VAL` | Vérifie une sonde. VAL vaut `N`, `0xN`, `<N` ou `>N` |

**Les noms :**
- **BTN :** `OCT- OCT+ FX SEL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC`.
- **ENC :** `SELECT ALGO K1 K2 K3 K4 PRESETS`.
- **K :** `0` à `26` (0 = F3), `w0` à `w15` pour les touches blanches, `b0` à `b10` pour les noires, ou une note de `F3` à `G5` (`C4`, `C#4` ou `C-4`).

**Les sondes de `expect` :** `ms`, `buttons`, `keys`, `enc ENC` (les crans que l'app n'a pas encore lus), `lit_buttons`, `lit_keys`, `dim_keys`, `play_red`, `blits`, `pixel X Y` (RGB565), `wav_frames`, `peak`, `midi_out`, `store_writes`, et l'état de l'app (`ui_probe()` de `ui/ui.h`) : `view`, `track`, `phrase`, `cursor row|col`, `octave`, `cell LIGNE COLONNE` (l'octet de la phrase éditée, colonne `STEP_*`). `blitsreset` remet le compteur `blits` à zéro, pour mesurer ce qu'un geste redessine.

## 7. Le budget CPU sans appareil (modèle de coût)

Tant qu'on n'a pas de FM-1, on ne peut pas mesurer le CPU réel. On applique donc ces garde-fous :

1. **Un compteur d'opérations, rudimentaire :** chaque moteur déclare un coût `cost_per_frame` en multiplications-accumulations (MAC) dans son descripteur. `voice.c` additionne le coût des voix actives et des effets, et `plat_cpu_pct()` le rapporte à un budget de référence.
2. **Le budget de référence :** on le fixe d'après les moteurs connus de Felucca, qui tient 8 voix plus ses effets. On prend une marge et on vise **≤ 50 %** avec 8 pistes actives et tous les effets.
3. **Dans le navigateur :** une jauge « CPU estimé » affiche cette valeur. Elle passe au rouge au-dessus de 50 %.
4. **En P5 :** on mesure avec `fm1_ticks()` autour de `app_audio` et on recalibre la référence.

C'est une estimation : elle sert à éviter les mauvaises surprises, pas à garantir l'absence de craquements.

Coûts déclarés (`voice_cost()`, en MAC par frame et par voix active) :

| Moteur | Coût | Détail |
| --- | --- | --- |
| `WAV` v1 (P1-03) | 12 | oscillateur 2, filtre SVF 5 (multiplications 64 bits), gains et panoramique 4, plus le contrôle tous les 16 frames |

## 8. Le portage sur l'appareil (P5, résumé)

1. Copier dans `firmware/` les éléments de Felucca : `hal/`, `loader/`, `app.ld`, `crt0.S`, `libc.c`, `tools/build.py`, `fm1pkg_make.py`, `get_toolchain.sh`. En-têtes GPL conservés, crédits dans `LICENSING.md`.
2. Écrire `plat_fm1.c` : les entrées par `fm1_input_edges`/`fm1_enc_take`, l'audio par le callback `fm1_alnk0_irq` qui appelle `app_audio`, et le stockage dans la zone de données (0x97000–0xE0000), écrit seulement à l'arrêt.
3. Prendre l'identité **FM-1_95N** (à confirmer), et remplacer le VID/PID USB de test.
4. Reprendre le mode secours (OCT− à l'allumage) et le garde-fou des deux crashs au démarrage, comme Dinghy.
5. Publier une release GitHub avec le `.fwsc`, le texte de la licence Apache des fichiers JieLi et le source, puis demander l'inscription sur le hub.
