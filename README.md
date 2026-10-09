# Trawler — un tracker pour le M-VAVE FM-1

> Nom de travail. Un firmware open source (GPL-3.0) qui fait du FM-1 un tracker de poche, dans l'esprit du M8 : Song → Chain → Phrase → Instrument → Table → Groove. **On le développe d'abord dans le navigateur.**

## Où en est-on

**Phase de product management terminée.** Le code n'a pas commencé : il sera écrit avec Claude Code, en suivant `docs/06_BACKLOG.md`.

## Le dossier

| Fichier | Contenu |
| --- | --- |
| `CLAUDE.md` | Les consignes permanentes pour Claude Code : règles de la puce, commandes, dossiers, références |
| `docs/01_FM1_HARDWARE.md` | Le FM-1 : puce, mémoire, contrôles, écran, chaîne de build, licence. Avec les sources |
| `docs/02_ECOSYSTEME.md` | Les 18 firmwares existants, leurs GitHub, comment ils tournent dans le navigateur, ce qui manque |
| `docs/03_PRD.md` | Vision, personas, périmètre du MVP, exigences, critères de succès, risques |
| `docs/04_SPEC_TRACKER.md` | Modèle de données, séquenceur, vues, contrôles du FM-1, moteurs, commandes FX, format `.trw` |
| `docs/05_ARCHITECTURE.md` | `plat.h`, les 3 cibles (hôte, web, appareil), builds, protocole du worklet, tests, budget CPU |
| `docs/06_BACKLOG.md` | Phases P0 à P5, stories et critères d'acceptation |
| `docs/07_PROMPTS_CLAUDE_CODE.md` | Les prompts à coller pour démarrer et pour la routine |
| `docs/08_DECISIONS.md` | Décisions prises (ADR) et questions ouvertes |

## Démarrer

```sh
cd ~/"Music/_Vibe Coding/FM1"
git init
claude          # puis coller le prompt 0 de docs/07_PROMPTS_CLAUDE_CODE.md
```

Il faut avoir installé :

- Docker Desktop, pour Emscripten (et plus tard pour la toolchain JieLi) ;
- Node 20 ou plus, pour le test de parité ;
- Python 3, pour les outils ;
- Chrome ou Edge, pour Web MIDI.

## Crédits et inspiration

Ce projet s'appuie sur le travail de la communauté FM-1, en particulier :

- Felucca (Leo Kuroshita / Hügelton Instruments) ;
- X0X et FoMni (Charles Vestal) ;
- Dinghy, zp12 et le hub (zvenson) ;
- ChoralRoot (Quixotic7) ;
- Hortator (DEADACTIVE).

M-VAVE et FM-1 sont des marques de leurs propriétaires. M8 est une marque de Dirtywave. Trawler n'est affilié à aucune de ces sociétés.
