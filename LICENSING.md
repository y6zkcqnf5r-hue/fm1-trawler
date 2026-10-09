# Licence de Trawler

Trawler est un logiciel libre sous la GNU General Public License, version 3 uniquement (`GPL-3.0-only`, texte complet dans `LICENSE`). Si vous distribuez Trawler, ou un firmware qui en dérive, vous devez fournir à vos destinataires le source complet correspondant, sous la même licence.

Chaque fichier source commence par `SPDX-License-Identifier: GPL-3.0-only`. Le code repris d'un autre projet garde son en-tête et sa licence d'origine, et il est listé ci-dessous.

## D'où vient le code

| Quoi | Origine | Licence |
| --- | --- | --- |
| Méthode de la table de hauteurs (une seule octave stockée, décalée pour les autres) : `tools/gen_tables.py`, `pitch_inc()` dans `firmware/src/dsp/fixed.h` | [Felucca](https://github.com/hugelton/Felucca) `tools/gen_tables.py`, Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments ; réécrit pour Trawler | GPL-3.0-only |
| Forme de l'interface `firmware/src/app/plat.h` | [X0X](https://github.com/charlesvestal/fm1-x0x) et [FoMni](https://github.com/charlesvestal/fm1-fomni) de Charles Vestal ; réécrite pour Trawler | GPL-3.0-only |
| Tout le reste | Trawler | GPL-3.0-only |

## Matériel tiers

Aucun pour l'instant.

## Marques

« M-VAVE » et « FM-1 » sont des marques de leurs propriétaires. « M8 » est une marque de Dirtywave. Trawler est un firmware indépendant : il n'est ni affilié à ces sociétés, ni approuvé, ni soutenu par elles.

## Radio

Trawler n'active jamais la radio Bluetooth ou Wi-Fi de l'appareil.
