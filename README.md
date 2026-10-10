# Trawler — un tracker pour le M-VAVE FM-1

[![ci](https://github.com/y6zkcqnf5r-hue/fm1-trawler/actions/workflows/ci.yml/badge.svg)](https://github.com/y6zkcqnf5r-hue/fm1-trawler/actions/workflows/ci.yml)

Un firmware libre (GPL-3.0) qui fait du FM-1, le petit synthé de poche de M-VAVE, un **tracker** : Song → Chain → Phrase → Instrument → Table → Groove, dans la lignée des trackers de poche comme LSDJ. On le développe d'abord dans le navigateur, avec **le même code C** que celui qui tournera sur l'appareil.

**▶ Le FM-1 virtuel : https://y6zkcqnf5r-hue.github.io/fm1-trawler/** : cliquez sur Démarrer, puis jouez au clavier de l'ordinateur ou sur le panneau. Le panneau indique quelle touche de votre clavier (QWERTY ou AZERTY) joue chaque contrôle.

## Où en est-on

- **P0, le squelette : fait.** Le code de l'appareil compilé en WebAssembly et joué dans un AudioWorklet, un panneau cliquable (souris, doigt, clavier), un simulateur hôte scripté qui rend des WAV et des PNG, et une CI qui vérifie que le wasm et le natif rendent les mêmes octets. L'app de démonstration joue un simple sinus.
- **P1, le cœur du tracker : à venir.** « Je tape une phrase et je l'entends. »

Le détail est dans [`docs/06_BACKLOG.md`](docs/06_BACKLOG.md).

## Construire et tester

Il faut `make`, un compilateur C, Python 3, Node 20 ou plus, et Emscripten 6.0.11 : soit un emsdk local dans `~/emsdk`, soit Docker (`DOCKER=1`, image `emscripten/emsdk:6.0.11`).

```sh
make test        # lint, tests unitaires, scénarios et leurs golden, parité wasm/natif
make web         # build/emu : la page, le worklet et trawler.wasm
make serve       # le FM-1 virtuel sur http://localhost:8080
make web DOCKER=1
```

Le code de `firmware/src/` suit les règles de la puce : entiers seulement, mémoire statique, pas de libc. Voir [`CLAUDE.md`](CLAUDE.md).

## Les documents

| Fichier | Contenu |
| --- | --- |
| [`docs/01_FM1_HARDWARE.md`](docs/01_FM1_HARDWARE.md) | Le FM-1 : puce, mémoire, contrôles, écran, chaîne de build. Avec les sources |
| [`docs/02_ECOSYSTEME.md`](docs/02_ECOSYSTEME.md) | Les autres firmwares du FM-1 et comment ils tournent dans le navigateur |
| [`docs/03_PRD.md`](docs/03_PRD.md) | Vision, périmètre du MVP, critères de succès |
| [`docs/04_SPEC_TRACKER.md`](docs/04_SPEC_TRACKER.md) | Modèle de données, séquenceur, vues, contrôles, commandes FX, format `.trw` |
| [`docs/05_ARCHITECTURE.md`](docs/05_ARCHITECTURE.md) | `plat.h`, les trois cibles (hôte, web, appareil), builds, protocole du worklet, tests |
| [`docs/06_BACKLOG.md`](docs/06_BACKLOG.md) | Les phases P0 à P5 et leurs stories |
| [`docs/08_DECISIONS.md`](docs/08_DECISIONS.md) | Les décisions (ADR) et les questions ouvertes |

## Crédits

Trawler s'appuie sur le travail de la communauté FM-1 :

- X0X et FoMni (Charles Vestal) : l'interface `plat.h`, le simulateur, l'émulateur web et son panneau ;
- Felucca (Leo Kuroshita / Hügelton Instruments) : la couche matérielle, pour le portage ;
- ChoralRoot (Quixotic7) : le test de parité wasm/natif ;
- Dinghy, zp12 et le hub (zvenson) : les règles de la puce ;
- Hortator (DEADACTIVE) : Emscripten figé en Docker.

Ce qui vient d'ailleurs est listé dans [`LICENSING.md`](LICENSING.md).

## Licence et marques

GPL-3.0-only, texte complet dans [`LICENSE`](LICENSE). « M-VAVE » et « FM-1 » sont des marques de leurs propriétaires ; « M8 » est une marque de Dirtywave. Trawler est un firmware indépendant : il n'est ni affilié à ces sociétés, ni approuvé, ni soutenu par elles.
