# CLAUDE.md — Trawler, un tracker pour le M-VAVE FM-1

> « Trawler » est un nom de travail (un chalutier, et un clin d'œil à *tracker*). Il peut changer, voir `docs/08_DECISIONS.md`.

## Ce qu'on construit

Un **firmware tracker** pour le synthé de poche M-VAVE FM-1, inspiré du workflow Song → Chain → Phrase → Instrument → Table → Groove des trackers comme le M8 ou LSDJ.

- **D'abord dans le navigateur :** le même code C que celui de l'appareil, compilé en WebAssembly, joué dans un AudioWorklet, sur un FM-1 virtuel (panneau SVG et écran 240×240 sur canvas).
- **Ensuite sur l'appareil :** on porte ce code en écrivant **un seul fichier** de plateforme (`plat_fm1.c`) au-dessus de la couche matérielle de Felucca.

Documents à lire avant toute tâche, dans cet ordre :

1. `docs/03_PRD.md` — vision, périmètre, critères de succès
2. `docs/04_SPEC_TRACKER.md` — modèle de données, vues, contrôles, commandes FX
3. `docs/05_ARCHITECTURE.md` — dossiers, `plat.h`, build, protocole du worklet, tests
4. `docs/06_BACKLOG.md` — l'ordre des tâches (on prend la prochaine story non faite)
5. `docs/01_FM1_HARDWARE.md` — contraintes de la puce (à relire avant tout code DSP)

## Règles non négociables

Le code dans `firmware/src/` doit tourner tel quel sur la puce. Ces règles valent donc **aussi pour la version navigateur** :

1. **Entiers seulement dans `firmware/src/`.** Pas de `float` ni de `double` : la puce JieLi AC791N n'a pas de FPU et la liaison échoue. On utilise du point fixe : Q15 pour les niveaux, Q12 pour les gains, une phase de 32 bits pour les oscillateurs. Les constantes se calculent sur l'ordinateur et s'écrivent en dur dans le source.
2. **Pas de division 64 bits.** On peut multiplier en `int64_t`, mais jamais faire `/` ou `%` sur 64 bits. On décale, ou on divise en 32 bits. Une division par zéro ne plante pas : elle donne une valeur fausse. Il faut donc tester les diviseurs.
3. **Pas de libc, sauf ce qu'on fournit.** Pas de `malloc`, de `printf` ni de `math.h` dans `firmware/src/`. Toute la mémoire est statique.
4. **Budgets mémoire du FM-1, à respecter dès le premier jour :**
   - RAM ≤ 96 Kio ;
   - pool ≤ 336 Kio (gros tampons marqués `APP_POOL`) ;
   - code ≤ ~560 Kio ;
   - un projet sérialisé ≤ 48 Kio.

   `tools/size_report.py` mesure ces chiffres sur le build hôte.
5. **L'audio est un callback de 256 frames à 44,1 kHz**, soit 5,8 ms sur l'appareil. On vise moins de la moitié. Dans ce callback :
   - pas d'accès à la flash ni à l'écran, pas d'attente ;
   - les enveloppes, les LFO et les coefficients de filtre se calculent par blocs de 16 ou 32 frames.
6. **Le code de l'app ne parle au matériel que par `firmware/src/app/plat.h`.** Aucun `#ifdef WEB` dans `firmware/src/dsp/` ni dans `firmware/src/tracker/`. Seuls `host/` et `web/emu/` savent qu'ils ne sont pas sur un FM-1.
7. **Wasm et natif doivent rendre le même son, échantillon par échantillon.** Le test `tests/host/emu_test.mjs` le vérifie, avec les mêmes flags (`-ffp-contract=off -fwrapv`).
8. **Licence GPL-3.0-only.** Chaque fichier commence par `// SPDX-License-Identifier: GPL-3.0-only`. Le code repris d'ailleurs garde son en-tête et sa licence d'origine, qui doit être compatible, et se note dans `LICENSING.md`.
9. **Pas de marque M8 ni Dirtywave dans le produit.** On ne copie ni leurs textes, ni leurs polices, ni leurs thèmes. Le vocabulaire courant des trackers (phrase, chain, table, groove, codes FX de 3 lettres) est libre d'usage.

## Commandes

```sh
make host        # build natif : build/host/trawler (simulateur scripté → WAV + PNG)
make web         # build wasm : build/emu/{index.html, worklet.js, trawler.wasm}
make serve       # sert build/emu sur http://localhost:8080 (AudioWorklet = contexte sécurisé : localhost OK)
make test        # tests unitaires hôte + scénarios + parité wasm/natif + rapport de taille
make lint        # vérifie : pas de float/double/malloc/printf dans firmware/src, en-têtes SPDX
```

Emscripten est figé en version via Docker (`emscripten/emsdk:<version>` dans le Makefile) pour que tout le monde obtienne le même `.wasm`. `emsdk` installé en local marche aussi, avec la même version.

## Dossiers

```
firmware/src/dsp/       moteurs sonores, filtres, effets — C99 pur, entiers
firmware/src/tracker/   modèle de données, séquenceur, commandes FX, sérialisation
firmware/src/ui/        vues (song, chain, phrase, instr, table, groove, mixer), gfx, police
firmware/src/app/       main, plat.h (l'interface), plat_fm1.c (plus tard, sur l'appareil)
host/                   plat.h pour l'hôte + lanceur de scénarios
web/emu/                wrapper wasm, worklet.js, index.html, panneau SVG, keymap.js
tests/                  unitaires, scenarios/*.trs, golden/, host/emu_test.mjs
tools/                  gen_font.py, gen_tables.py, size_report.py, lint.sh
vendor-ref/             clones en LECTURE SEULE des projets de référence (gitignore)
docs/                   PRD, spec, archi, backlog, décisions
```

## Projets de référence (à cloner dans `vendor-ref/`, ne pas modifier)

| Repo | Ce qu'on y prend |
| --- | --- |
| `charlesvestal/fm1-fomni` | **Le modèle à suivre.** `firmware/src/app/plat.h`, `host/omni_host.c`, `web/emu/` (build, worklet, panneau, clavier) |
| `charlesvestal/fm1-x0x` | Le gabarit d'émulateur d'origine, le lanceur de scénarios |
| `hugelton/Felucca` | Couche matérielle `firmware/hal/`, loader, `tools/build.py`, `fm1pkg_make.py`, installeur web — pour la phase 5 |
| `zvenson/dinghy` | `docs/API.md`, `RULES.md`, `MAKE-IT-YOURS.md` : les règles de la puce, l'identité produit |
| `Quixotic7/ChoralRootFM1` | Test de parité wasm/natif (`tools/emu/web/test_web_emu.mjs`), MIDI web entrée et sortie |
| `deadactive/hortator` | Docker emsdk figé, garde-fou contre la dérive des sources |
| `zvenson/zp12` | Exemple de CLAUDE.md pour un firmware FM-1 |

Tous ces projets sont en GPL-3.0 : on peut copier du code à condition de garder les en-têtes et de citer l'auteur dans `LICENSING.md`.

## Façon de travailler

- **Une story du backlog à la fois.** Mode plan d'abord, puis le code, puis `make test` au vert, puis un commit dont le message cite l'ID de la story (ex. `P1-03: phrase view cursor`).
- Avant tout nouveau moteur DSP, écrire un test qui rend un WAV de référence et vérifier son hash (golden).
- Les tables précalculées (sinus, notes → incréments de phase, etc.) se génèrent avec `tools/gen_tables.py` dans `build/gen/`, jamais à la main.
- Si une règle de ce fichier gêne, on ne la contourne pas : on le signale et on propose une décision à ajouter dans `docs/08_DECISIONS.md`.
- On ne promet pas de tenir un budget CPU qu'on ne peut pas mesurer. Tant qu'il n'y a pas de FM-1, on s'en tient au modèle de coût de `docs/05_ARCHITECTURE.md` §7.
- Langue : documentation et commits en français ; identifiants et commentaires de code en anglais.
