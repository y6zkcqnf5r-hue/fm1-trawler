# 03 — PRD : Trawler

| | |
| --- | --- |
| Product owner | Anthony |
| Statut | Brouillon v0.1, 2026-10-10 |
| Développement | Claude Code, guidé par ce dossier |

## 1. Le problème

Le FM-1 est devenu en quelques semaines une plateforme ouverte : 18 firmwares open source, un hub d'installation en un clic, des émulateurs dans le navigateur. Pourtant, **aucun firmware ne propose un tracker**. Or les trackers (M8, LSDJ, Polyend Tracker) ont une communauté nombreuse et fidèle, qui aime justement composer sur un petit écran avec quelques boutons.

Le FM-1 réunit les conditions pour accueillir un tracker de poche :

- un écran couleur de 240×240 ;
- 7 encodeurs ;
- 27 touches.

Ses atouts face au M8 :

- environ 80 € au lieu de plusieurs centaines ;
- un vrai clavier pour entrer les notes en les jouant, au lieu de les composer avec des flèches.

## 2. La vision

> **Trawler fait du FM-1 un tracker de poche : on compose en Song → Chain → Phrase → Table comme sur un M8, mais on joue les notes sur un vrai clavier et on tape les valeurs hexadécimales sur les 16 touches blanches. Il marche d'abord dans le navigateur, sans appareil.**

## 3. Pour qui

| Persona | Besoin | Ce qui le fait venir |
| --- | --- | --- |
| **Le trackeur** (Anthony et la communauté M8/LSDJ) | Retrouver le flux de travail Song/Chain/Phrase/Table sur un appareil peu cher et transportable | Un modèle fidèle, les commandes FX, les tables, les grooves |
| **Le propriétaire de FM-1 curieux** | Essayer un autre instrument en un clic | Installation depuis le hub, projet de démo jouable dès l'allumage |
| **Le curieux sans FM-1** | Essayer avant d'acheter | Le FM-1 virtuel dans le navigateur, au clavier de l'ordinateur |

Styles visés en priorité : **synthwave et lofi**. Ils guident le choix des moteurs (VA avec detune, FM, batterie synthétique) et des effets (chorus, delay, réverbe, saturation tape, bitcrush).

## 4. Le périmètre

### Le MVP (navigateur, phases 0 à 4 du backlog)

- **Structure :**
  - Song à 8 pistes ;
  - 128 chains ;
  - 128 phrases de 16 pas, avec les colonnes note, volume, instrument et FX1–FX3 ;
  - 32 instruments ;
  - 64 tables ;
  - 16 grooves.
- **Vues :** Song, Chain, Phrase, Instrument, Table, Groove, Mixer et Projet, avec une mini-carte de navigation.
- **Moteurs :**
  - `WAV`, un oscillateur VA ou wavetable avec detune et filtre SVF ;
  - `FM4`, une FM à 4 opérateurs ;
  - `DRM`, une batterie synthétique.
- **Effets :** environ 25 commandes FX (liste dans la spec §6).
- **Mixer :**
  - envois vers chorus, delay et réverbe ;
  - bus master avec saturation tape et bitcrush ;
  - volume et panoramique par piste.
- **Édition :** sélection, copier/couper/coller, clonage d'une phrase ou d'une chain, annulation sur 16 niveaux.
- **Lecture :** jouer la song, la chain ou la phrase en cours. Saisie en direct avec REC. Tempo de 40 à 300 BPM.
- **FM-1 virtuel :**
  - panneau cliquable et tactile ;
  - clavier de l'ordinateur ;
  - Web MIDI en entrée ;
  - écran 240×240 agrandi ×2.
- **Fichiers :** projet sauvegardé dans le navigateur, export et import d'un fichier `.trw`, **export WAV** de la song.
- **Démos :** 2 projets, un synthwave et un lofi, et une aide en surimpression qui montre les contrôles de la vue en cours.

### Hors MVP (phases 5 et suivantes)

- Portage sur l'appareil, paquet `.fwsc`, publication sur le hub.
- Sampler (la mémoire est très limitée sur l'appareil), MIDI Out, gammes, moteur FM6 compatible Dexed, thèmes, partage d'un projet par URL, Web MIDI en sortie.

### Ce qu'on ne fera pas

- Une compatibilité avec les fichiers du M8.
- Une copie de l'interface, des polices ou des textes du M8.
- Du Bluetooth.

## 5. Exigences non fonctionnelles

| Exigence | Cible | Comment on la vérifie |
| --- | --- | --- |
| Même code sur le web et l'appareil | 100 % du code de `firmware/src` est commun | Le lint interdit `#ifdef WEB` dans `firmware/src` |
| Parité du son | WAV identiques à l'octet près entre wasm et natif | `tests/host/emu_test.mjs` |
| Entiers seulement | 0 `float` ou `double` dans `firmware/src` | `make lint` |
| Mémoire | RAM ≤ 96 Kio, pool ≤ 336 Kio, projet ≤ 48 Kio | `tools/size_report.py` en CI |
| CPU | 8 pistes en lecture ≤ 50 % du budget estimé | Modèle de coût (archi §7), mesure réelle en phase 5 |
| Latence dans le navigateur | De la touche au son ≤ 30 ms | Blocs de 128 frames dans l'AudioWorklet |
| Robustesse | Pas de perte de projet en rechargeant la page | Sauvegarde automatique dans IndexedDB toutes les 5 s quand le projet a changé |
| Navigateurs | Chrome, Edge, Firefox et Safari récents, ordinateur et iPad | Tests manuels, checklist en P4 |

## 6. Critères de succès

1. **Le parcours « premier son » :** quelqu'un qui découvre le FM-1 virtuel entend une phrase qu'il a tapée lui-même **en moins de 5 minutes**, avec l'aide en surimpression.
2. **Le parcours « vrai morceau » :** Anthony compose une boucle synthwave d'une minute sur 4 pistes **en moins de 45 minutes**, entièrement dans Trawler.
3. Tous les critères de la §5 sont au vert dans la CI.
4. **Phase 5 :** le `.fwsc` s'installe depuis le hub et le projet de démo joue sans craquement sur un vrai FM-1.

## 7. Risques

| Risque | Effet | Parade |
| --- | --- | --- |
| Le CPU de l'appareil est inconnu tant qu'on n'a pas de FM-1 | Une fois porté, tout peut craquer | Moteurs peu gourmands et monophoniques par piste, modèle de coût, nombre de pistes réglable par une constante, acheter un FM-1 avant la P5 |
| L'écran 240×240 est étroit pour 3 colonnes FX | La lisibilité en souffre | Police de 6×10 (ou 8×12) à tester en P1. Repli : afficher FX3 en défilant |
| Pas de bouton dédié pour éditer en hexa | Saisie lente | Les 16 touches blanches servent de clavier hexa (spec §4.3), à tester en P1 |
| Une sauvegarde en flash coupe le son | Clics audio | Sauvegarde seulement à l'arrêt (phase 5) |
| Écrire un tracker est un gros chantier | Dérive du planning | Backlog en tranches verticales : chaque phase produit quelque chose de jouable |
| L'identité produit entre en collision avec un autre firmware | Le hub refuse le paquet | Demander une plage à zvenson avant la P5 |
