# 02 — Les firmwares existants pour le FM-1

Source : le catalogue du hub [fm1.designburgapps.com/catalog.json](https://fm1.designburgapps.com/catalog.json), mis à jour le 2026-10-09, complété par les README des projets. Les étoiles GitHub sont celles du catalogue.

## 1. Le catalogue (18 firmwares open source, tous en GPL-3.0)

| Firmware | Auteur · repo | Genre | Version | ★ | Dans le navigateur | En une phrase |
| --- | --- | --- | --- | --- | --- | --- |
| **Felucca** | Leo Kuroshita · [hugelton/Felucca](https://github.com/hugelton/Felucca) | synthé, groove, dev | 1.5 | 344 | [try](https://hugelton.github.io/Felucca/webapp/try/) · [éditeur](https://hugelton.github.io/Felucca/webapp/editor/) | La base de tous les autres : 13 moteurs (VA, FM6/Dexed, PD, grain, Rings…), 4 pistes, séquenceur de 64 pas |
| Felucca [Salt] | Chance Roth · [ChanceTheMaker/Felucca](https://github.com/ChanceTheMaker/Felucca) | synthé | 0.9-salt14 β | 3 | [Studio](https://chancethemaker.github.io/Felucca/webapp/editor/) | Felucca avec un Studio web et des thèmes. Contient un fichier AGENTS.md |
| WaveLoop | ELI7VH · [ELI7VH/Felucca](https://github.com/ELI7VH/Felucca) | synthé | 1.0.5.2-midi21 | 2 | [éditeur](https://eli7vh.github.io/Felucca/webapp/editor/) | Felucca piloté par un Arturia MiniLab 3 |
| **SLOOP** | isod89 · [isod89/sloop-fm1](https://github.com/isod89/sloop-fm1) | groovebox | 2.5 | 321 | [éditeur](https://isod89.github.io/sloop-fm1/webapp/editor/) | Groovebox live, 4 pistes, avec des « couches » : on maintient un bouton et les touches changent de rôle |
| Bubba | Erbubar23 · [Erbubar23/sloop-fm1](https://github.com/Erbubar23/sloop-fm1) | groovebox | 2.0 | 3 | éditeur | SLOOP à 8 pistes, sampler avec découpe, looper sur chaque piste |
| SLOOP Floyd FM | René Bohne · [renebohne/sloop-fm1](https://github.com/renebohne/sloop-fm1) | synthé | 2.4 | 0 | — | SLOOP avec une FM à 4 opérateurs et un oscilloscope des enveloppes |
| SLOOP DX7 Banks | majnikool · [majnikool/sloop-fm1](https://github.com/majnikool/sloop-fm1) | synthé | 2.5-b28 β | 0 | — | SLOOP avec jusqu'à 12 banques DX7 |
| sloopDX | zvenson · [zvenson/dxsloop](https://github.com/zvenson/dxsloop) | groovebox | 3.4 | 16 | [éditeur](https://dx7.designburgapps.com/webapp/editor/) | SLOOP avec uniquement Dexed, 128 pas, batterie FM. **Contient un CLAUDE.md** |
| zp12 | zvenson · [zvenson/zp12](https://github.com/zvenson/zp12) | batterie | 2.1 | 14 | [éditeur d'échantillons](https://zp12.designburgapps.com/editor/) | Sampler 12 bits façon SP-1200. **Contient un CLAUDE.md** |
| Hortator | DEADACTIVE · [deadactive/hortator](https://github.com/deadactive/hortator) | batterie | 0.14 β | 3 | [simulateur](https://deadactive.github.io/hortator/) | Boîte à rythmes 8 pistes, Grids, compresseur. Simulateur wasm d'une conception différente |
| **X0X** | Charles Vestal · [charlesvestal/fm1-x0x](https://github.com/charlesvestal/fm1-x0x) | groovebox | 0.10.3 β | 90 | [émulateur](https://charlesvestal.github.io/fm1-x0x/emu/) | 909/808 et deux 303. **C'est lui qui a lancé le gabarit d'émulateur web.** Pas encore testé sur l'appareil |
| **FoMni** | Charles Vestal · [charlesvestal/fm1-fomni](https://github.com/charlesvestal/fm1-fomni) | accords | 0.3.2 β | 41 | [émulateur](https://charlesvestal.github.io/fm1-fomni/emu/) | Harpe d'accords façon Omnichord. **L'interface `plat.h` la plus propre.** Commits co-signés par Claude |
| FiMba-1 | jadamsowers · [jadamsowers/fm1-fimba](https://github.com/jadamsowers/fm1-fimba) | synthé, accords | 0.6 β | 5 | [lecteur](https://jadamsowers.github.io/fm1-fimba/) | Kalimba en modèle physique. Fork de FoMni. **Fait avec Claude Code**, CLAUDE.md, CI qui publie les releases |
| ChoralRoot | Quixotic7 · [Quixotic7/ChoralRootFM1](https://github.com/Quixotic7/ChoralRootFM1) | accords | 0.13 β | 12 | [émulateur](https://quixotic7.github.io/ChoralRootFM1/emu/) | Instrument d'accords façon Telepathic Orchid. **Test de parité wasm/natif**, MIDI web en entrée et en sortie |
| Jangada | zednaked · [zednaked/jangada](https://github.com/zednaked/jangada) | synthé | 0.8.2 β | 19 | [Studio](https://zednaked.github.io/jangada/webapp/studio/) | Drones sombres, VOSIM, GENDYN, matrice de modulation. Studio wasm compilé avec zig |
| GHOULBOX | Jason Persinger · [jasonpersinger/ghoulbox…](https://github.com/jasonpersinger/ghoulbox-fm1-dungeon-synth) | synthé | 1.1 | 15 | éditeur | « Dungeon synth », vielle à roue, réverbe de cathédrale. Commits co-signés par Claude Opus 5.5 |
| Melodee | keremimo · [keremimo/melodee](https://github.com/keremimo/melodee) | synthé | 0.13.1 β | 30 | [éditeur](https://keremimo.github.io/melodee/webapp/editor/) | Prophet-5, CZ-1, Dexed exact, enregistrement non quantifié, 54 gammes microtonales |
| Dinghy | zvenson · [zvenson/dinghy](https://github.com/zvenson/dinghy) | dev | 0.3 β | 0 | — | **Squelette minimal documenté** pour écrire son propre firmware |

Deux firmwares fermés, donc absents du hub : [Baud Girl FM-1+VA](https://baudgirl.com/work/FM-1+VA/install) et [Groove OS](https://www.groove-os.com/) (29 $, construit sur la V15, a un émulateur web).

## 2. La généalogie

```
Felucca (hugelton) ── base matérielle, loader, installeur web
 ├─ SLOOP (isod89) ── Bubba, Floyd FM, DX7 Banks, sloopDX ─ zp12 ─ Dinghy (zvenson)
 ├─ X0X (charlesvestal) ── FoMni ── FiMba-1           ← gabarit d'émulateur « plat.h »
 ├─ ChoralRoot, Hortator, Salt ── Jangada, GHOULBOX, Melodee, WaveLoop
```

## 3. Comment ils tournent dans le navigateur

Aucun n'émule la puce, et aucun ne réécrit le son en JavaScript. **Tous compilent leur propre code C en WebAssembly et le jouent dans un AudioWorklet.** Ce qui les distingue, c'est l'endroit où ils séparent le code du matériel :

| Niveau | Projets | Où passe la frontière | Pour Trawler |
| --- | --- | --- | --- |
| **A — interface de l'app** | X0X, FoMni, FiMba | Environ 15 fonctions `plat_*`, implémentées une fois pour l'hôte et une fois pour l'appareil | ✅ **Retenu** (ADR-002) |
| B — remplacement de la couche matérielle | Felucca /try, ChoralRoot, Hortator | Les fonctions `fm1_*` de bas niveau sont remplacées : matrice de touches, image de 1 Mio de flash | Trop lié à Felucca |
| C — son seul | Salt, Jangada | Seuls les moteurs passent en wasm ; l'interface est une page HTML à part | Pas de FM-1 virtuel |

Les bonnes idées à reprendre :

- FoMni : `plat.h` et le lanceur de scénarios (`press`, `turn`, `wav`, `shot`, `expect`).
- ChoralRoot : le test de parité wasm/natif, et le MIDI web en entrée et en sortie.
- Felucca : maintenir chaque appui au moins 24 ms pour qu'il passe l'anti-rebond.
- Hortator : emsdk figé dans Docker.
- FiMba : une version en un seul fichier HTML qui s'ouvre sans serveur.
- Salt : rééchantillonner si le navigateur refuse 44,1 kHz.
- ChoralRoot : démarrer une instance muette pour afficher l'écran avant le premier clic.

## 4. Les habitudes d'interface qui marchent sur un si petit instrument

1. **Maintenir un bouton = une couche** où les touches et les potards changent de rôle. On la verrouille avec HOME ou un double appui (SLOOP, Felucca, Jangada).
2. **Les 16 touches blanches pour 16 pas ou 16 valeurs, les 11 noires pour des rôles secondaires** (pistes, accords, mutes).
3. **Quatre cases à l'écran, une par potard, de la même couleur que le potard**, avec un grand affichage de la valeur quand on tourne.
4. **OCT+ pour valider, OCT− pour revenir** (Felucca). Pour Trawler, on garde OCT± pour l'octave du clavier, voir la spec.
5. **Les changements tombent sur la mesure suivante**, sauvegarde automatique pendant le silence, confirmation avant toute action destructrice, mode secours.
6. **Démarrer directement sur une scène jouable** (GHOULBOX, FoMni).

## 5. Ce que personne n'a fait (et la place de Trawler)

- **Aucun tracker sur le FM-1.** C'est la place de Trawler.
- Pas de wavetable ni de Plaits, pas de looper audio, pas de séquences génératives en page d'accueil, pas d'export MIDI ou WAV, pas de partage de motifs.
- Trawler peut en reprendre deux en route : l'export WAV depuis le navigateur, et le partage de projets.
