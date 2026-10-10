# Licence de Trawler

Trawler est un logiciel libre sous la GNU General Public License, version 3 uniquement (`GPL-3.0-only`, texte complet dans `LICENSE`). Si vous distribuez Trawler, ou un firmware qui en dérive, vous devez fournir à vos destinataires le source complet correspondant, sous la même licence.

Chaque fichier source commence par `SPDX-License-Identifier: GPL-3.0-only`. Le code repris d'un autre projet garde son en-tête et sa licence d'origine, et il est listé ci-dessous.

## D'où vient le code

| Quoi | Origine | Licence |
| --- | --- | --- |
| Méthode de la table de hauteurs (une seule octave stockée, décalée pour les autres) : `tools/gen_tables.py`, `pitch_inc()` dans `firmware/src/dsp/fixed.h` | [Felucca](https://github.com/hugelton/Felucca) `tools/gen_tables.py`, Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments ; réécrit pour Trawler | GPL-3.0-only |
| Forme de l'interface `firmware/src/app/plat.h` | [X0X](https://github.com/charlesvestal/fm1-x0x) et [FoMni](https://github.com/charlesvestal/fm1-fomni) de Charles Vestal ; réécrite pour Trawler | GPL-3.0-only |
| Simulateur hôte (`host/`) : horloge `host_run_ms`, langage de scénarios `.trs`, PNG en blocs deflate « stored » sans zlib | [FoMni](https://github.com/charlesvestal/fm1-fomni) `host/omni_host.c` et [X0X](https://github.com/charlesvestal/fm1-x0x) `host/x0x_host.c` de Charles Vestal ; réécrit pour Trawler | GPL-3.0-only |
| Panneau `web/emu/panel.js` : géométrie du FM-1 (viewBox 1000×640), SVG sous des zones de clic HTML, potards au glisser et à la molette, verrou des boutons | [X0X](https://github.com/charlesvestal/fm1-x0x) et [FoMni](https://github.com/charlesvestal/fm1-fomni) `web/emu/index.html` de Charles Vestal ; réécrit pour Trawler (légendes gravées, légende du clavier par disposition, appui long) | GPL-3.0-only |
| Build web (`web/emu/trawler_web.c`, `worklet.js`, `index.html`) : wasm sans glue Emscripten et faux WASI, exports `web_*`, anneau audio rempli par l'horloge de l'appareil, messages `load` / `frame`, conversion de l'écran, clavier relâché sur `blur` | [X0X](https://github.com/charlesvestal/fm1-x0x) et [FoMni](https://github.com/charlesvestal/fm1-fomni) `web/emu/` de Charles Vestal ; réécrit pour Trawler | GPL-3.0-only |
| Test de parité `tests/host/emu_test.mjs` : le module chargé dans node avec les bouchons du worklet, comparaison échantillon par échantillon avec le natif | [ChoralRootFM1](https://github.com/Quixotic7/ChoralRootFM1) `tools/emu/web/test_web_emu.mjs` (Quixotic7) ; réécrit pour Trawler | GPL-3.0-only |
| CI `.github/workflows/ci.yml` : actions épinglées par commit, Pages par `upload-pages-artifact` et `deploy-pages` | [ChoralRootFM1](https://github.com/Quixotic7/ChoralRootFM1) `.github/workflows/pages.yml` (Quixotic7) ; réécrit pour Trawler | GPL-3.0-only |
| Emscripten en Docker sous l'utilisateur courant, cache dans le dossier de build (`Makefile`, `DOCKER=1`) | [Hortator](https://github.com/deadactive/hortator) `tools/build_sim.sh` (DEADACTIVE) | GPL-3.0-only |
| Générateur de police `tools/gen_font.py` (lecture du BDF), `firmware/src/ui/gfx.c` | Trawler | GPL-3.0-only |
| Tout le reste | Trawler | GPL-3.0-only |

## Matériel tiers

| Quoi | Origine | Licence | Fichiers |
| --- | --- | --- | --- |
| Police Terminus 12 px (6×12), la police provisoire de P0-04 | Copyright (C) 2020 Dimitar Toshkov Zhekov, copiée depuis [X0X](https://github.com/charlesvestal/fm1-x0x) `assets/fonts/` | SIL OFL 1.1 | `assets/fonts/ter-u12n.bdf`, `assets/fonts/Terminus-LICENSE.txt` ; les glyphes générés dans `build/gen/font.h` restent sous OFL |

## Marques

« M-VAVE » et « FM-1 » sont des marques de leurs propriétaires. « M8 » est une marque de Dirtywave. Trawler est un firmware indépendant : il n'est ni affilié à ces sociétés, ni approuvé, ni soutenu par elles.

## Radio

Trawler n'active jamais la radio Bluetooth ou Wi-Fi de l'appareil.
