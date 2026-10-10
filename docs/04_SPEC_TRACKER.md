# 04 — Spécification fonctionnelle de Trawler

Tout ce qui est marqué **[PROTO]** est une proposition à valider dans le FM-1 virtuel avant de la figer. La story correspondante dans le backlog prévoit ce test.

## 1. Le modèle de données

Toutes les valeurs tiennent sur 1 octet, et l'éditeur les affiche en hexadécimal. La valeur `0xFF` veut dire « vide ».

```
SONG     128 lignes × 8 pistes  → n° de chain (00–7F) ou vide
CHAIN    128 × 16 lignes        → (n° de phrase, transposition signée)
PHRASE   128 × 16 pas           → NOTE, VOL, INSTR, FX1, FX2, FX3
INSTR     32                    → type de moteur + paramètres + table par défaut
TABLE     64 × 16 lignes        → TRANSP, VOL, FX1, FX2, FX3 ; plus TIC (vitesse)
GROOVE    16 × 16 entrées       → nombre de ticks par pas
MIXER     1                     → volume et panoramique par piste, mute/solo, envois, master
PROJECT   1                     → nom, tempo, version du format
```

### Structures C (`firmware/src/tracker/model.h`)

Les limites (`TRK_*`) sont des `#define` de `firmware/src/app/config.h`, qu'on peut ajuster si la mémoire l'exige. Chaque taille annoncée ci-dessous est vérifiée par un `_Static_assert`.

```c
#define TRK_TRACKS     8
#define TRK_SONG_ROWS  128
#define TRK_CHAINS     128
#define TRK_PHRASES    128
#define TRK_INSTRS     32
#define TRK_TABLES     64      /* 00–1F : table par défaut de l'instrument de même numéro ; 20–3F : libres */
#define TRK_GROOVES    16
#define TRK_ROWS       16
#define TRK_EMPTY      0xFF
#define NOTE_OFF       0xFE    /* affiché « OFF » : passe la voix en relâchement */

typedef struct { uint8_t cmd, val; } fx_t;                    /* cmd 0 = aucune */
typedef struct { uint8_t note, vol, instr; fx_t fx[3]; } step_t;   /* 9 o ; note 0–119 = C-0…B-9 */
typedef struct { step_t s[TRK_ROWS]; } phrase_t;              /* 144 o */
typedef struct { uint8_t phrase[TRK_ROWS]; int8_t transp[TRK_ROWS]; } chain_t;  /* 32 o */
typedef struct { int8_t transp; uint8_t vol; fx_t fx[3]; } trow_t;               /* 8 o */
typedef struct { trow_t r[TRK_ROWS]; uint8_t tic; } table_t;  /* 129 o */
typedef struct { uint8_t ticks[TRK_ROWS]; } groove_t;         /* 16 o ; 0xFF = retour au début */
typedef struct { uint8_t type; char name[8]; uint8_t table, tic; uint8_t p[53]; } instr_t; /* 64 o */
```

Le mixer et l'en-tête du projet (ADR-022). Tout gain sur 8 bits vaut **v/0x80 : 80 = 0 dB, FF ≈ +6 dB** ; un panoramique va de 00 (gauche) à FF (droite), 80 au centre. Les envois par piste restent sur l'instrument (`SCH SDL SRV`, §5) ; le mixer porte les retours et les réglages des effets. Les valeurs par défaut des effets sont provisoires jusqu'à P3-05 et P3-06.

```c
typedef struct { uint8_t vol, pan; } mtrack_t;                    /* défaut 80, 80 */
typedef struct {
    mtrack_t track[TRK_TRACKS];          /* 16 o */
    uint8_t mute, solo;                  /* bit t = piste t ; 00 */
    uint8_t ret_cho, ret_del, ret_rev;   /* retours d'effet ; 80 */
    uint8_t cho_rate, cho_depth;         /* 20, 40 */
    uint8_t del_time, del_fb, del_hp;    /* temps en pas (MDT) 03, réinjection (MDF) 60, passe-haut 20 */
    uint8_t rev_size, rev_damp;          /* taille (MRS) A0, amortissement 60 */
    uint8_t master, tape, crush, limit;  /* 80, tape (MTP) 00, crush (MCR, 00 = coupé) 00, limiteur 01 */
} mixer_t;                               /* 32 o */
typedef struct {
    uint16_t version;                    /* version du format, 1 */
    uint16_t tempo;                      /* 40–300 BPM, défaut 120 */
    char name[12];                       /* ASCII 32–126, complété par des 0 ; vide par défaut */
} phdr_t;                                /* 16 o */
typedef struct {
    phdr_t hdr; mixer_t mixer;
    uint8_t song[TRK_SONG_ROWS][TRK_TRACKS];
    chain_t chain[TRK_CHAINS]; phrase_t phrase[TRK_PHRASES]; table_t table[TRK_TABLES];
    instr_t instr[TRK_INSTRS]; groove_t groove[TRK_GROOVES];
} project_t;                             /* 34 160 o */
```

**Projet vide** (`model_init`) : tout est `FF`, sauf les commandes FX et leurs valeurs (00), les transpositions (00), le TIC des tables et des instruments (01) ; le groove 00 vaut `06 06`, puis `FF` ; l'instrument `n` a pour table `n` et pour type `FF` (aucun moteur) ; la VOL d'une ligne de table vide (`FF`) ne change rien.

**Accès (ADR-021) :** l'audio lit le projet directement, par une vue `const` ; toute écriture passe par les fonctions `*_set` de `model.c`, qui vérifient les bornes et sont le seul point que verront l'annulation (P2-06) et la sauvegarde automatique (P2-08). Le numéro d'une colonne est le décalage de son octet dans la struct (`STEP_NOTE`… `STEP_FX3_VAL`). Le **premier numéro libre** (EDIT deux fois, §4.2) est la première chain, ou phrase, vide et que rien ne référence. Une groove refuse 00 tick.

Un projet pèse **34 160 octets (33,4 Kio)** :

| Bloc | Taille |
| --- | --- |
| Phrases | 18 432 o |
| Tables | 8 256 o |
| Chains | 4 096 o |
| Instruments | 2 048 o |
| Song | 1 024 o |
| Grooves, mixer, en-tête | 256 + 32 + 16 = 304 o |

C'est sous le plafond de 48 Kio, que vérifient un `_Static_assert` et `make size`. En mémoire, le projet vit dans le pool. Le format sérialisé `.trw` est décrit au §8.

## 2. Le séquenceur

### Horloge

- 24 ticks par noire. Par défaut, un pas dure 6 ticks, soit une double croche.
- Un tick dure exactement **110 250 / bpm échantillons** (44 100 × 60 / 24) : 918,75 à 120 BPM, donc 22 050 par noire et 5 512,5 par pas de 6 ticks (ADR-023, qui corrige ADR-010).

  ```
  q = 110250 / bpm        r = 110250 % bpm        (32 bits, seulement quand le tempo change)
  ```

  Chaque tick dure `q` échantillons, plus un quand l'accumulateur du reste (`acc += r`) atteint `bpm`. Le tick k tombe donc **exactement** sur l'échantillon `floor(k × 110250 / bpm)` : aucune dérive, à tous les tempos (`tests/unit/test_seq.c` le vérifie sur 10 min, de 40 à 300 BPM). Un nouveau tempo prend effet au tick suivant.
- **L'horloge est exacte à l'échantillon près :** le callback audio découpe son bloc de 256 frames aux frontières de tick, et traite les événements entre deux morceaux.
- Tempo : 40 à 300 BPM, entier pour le MVP.

### Lecture de la song

- Chaque piste lit sa colonne indépendamment des autres : ligne de song → chain → pas de phrase.
- Une chain se termine à sa première ligne vide ou après sa 16ᵉ ligne. La piste passe alors à la ligne de song suivante.
- Quand une piste arrive sur une **cellule vide**, elle revient à la première ligne du bloc de lignes non vides où elle se trouve. Les pistes de longueurs différentes se décalent ainsi naturellement.
- Selon la vue, PLAY lance :

  | Vue | Ce qui joue |
  | --- | --- |
  | Song | La song à partir de la ligne du curseur |
  | Chain | La chain en boucle |
  | Phrase | La phrase en boucle |
  | N'importe laquelle, avec SEL+PLAY | La song à partir de la ligne de song en cours |

- La durée de chaque pas vient du **groove** de la piste : la valeur de l'entrée `i` donne le nombre de ticks du pas `i`. `FF` renvoie à l'entrée 0 ; un groove entièrement vide vaut 6 ticks, pour que la lecture avance toujours.

### Déclenchement d'un pas (au tick 0)

1. Les commandes de séquence s'exécutent d'abord : `DEL`, `CHA` et `NTH` peuvent retarder ou annuler le pas.
2. Ensuite, selon le contenu du pas :

   | Le pas contient | Effet |
   | --- | --- |
   | NOTE et INSTR | La note se déclenche. Son instrument démarre sa table depuis la ligne 0 |
   | NOTE sans INSTR | La hauteur change **sans redéclencher** la note (legato). Avec `PSL`, la note glisse |
   | `OFF` | La voix passe en relâchement |
   | VOL | Fixe le volume de la note (00–FF). Vide avec une note : FF. Seul : change le volume de la note qui sonne |

   Une NOTE sans INSTR sur une voix muette la déclenche quand même.

3. **Une voix par piste.** Une nouvelle note sur une voix qui sonne la fond à 0 en 96 frames (6 sous-blocs de 16, 2,2 ms), puis part de la phase 0, sans clic ; sur une voix muette, elle part sur l'échantillon exact du tick. Au total, il n'y a donc jamais plus de 8 voix. L'enveloppe avance par sous-blocs de 16 frames comptés par la voix elle-même : le son ne dépend pas de la façon dont le bloc audio est découpé.
4. Les commandes continues (`ARP`, `PBN`, `VIB`, `PSL`, `RET`) agissent à chaque tick.

### Les tables

- Une table avance d'une ligne tous les `TIC` ticks (01 = à chaque tick) et boucle après la ligne F, ou plus tôt avec `HOP`.
- À chaque ligne, elle ajoute sa transposition à la note, multiplie le volume par VOL/0x80, et exécute ses FX comme une phrase.
- Elle redémarre à chaque nouvelle note. `TBL xx` change la table d'un instrument depuis une phrase.

## 3. Les vues

L'écran fait 240×240 pixels :

| Zone | Position | Contenu |
| --- | --- | --- |
| En-tête | y 0–19 | Nom de la vue et numéro, piste, ▶/■, tempo, octave du clavier, mini-carte |
| Grille | y 24–215 | 16 lignes de 12 px |
| Pied | y 216–239 | 4 cases colorées pour KNOB1–4 et une ligne d'aide |

**[PROTO]** Police : 6×10 ou 8×12, à choisir en P1. En 8×12, une ligne de phrase fait 30 caractères, soit 240 px tout juste.

```
PHRASE 1A   T3  ▶ 120  O4   [map]
0 C-4 7F 03 ARP37 CUT40 ---00
1 --- -- -- ----- ----- -----
2 D#4 60 03 PSL04 ----- -----
...
F OFF -- -- ----- ----- -----
[CUT 40][RES 20][DEL 10][REV 30]
```

| Vue | Bouton | Contenu |
| --- | --- | --- |
| **Song** | `HOME` | 8 colonnes de n° de chain, ligne de lecture surlignée par piste, numéros de ligne en hexa |
| **Chain** | `ARP` | 16 lignes : PHRASE, TRANSP |
| **Phrase** | `SEQ` | 16 pas : NOTE, VOL, INSTR, FX1–FX3 |
| **Instrument** | `ENV` | Type de moteur, nom, paramètres du moteur, filtre, ampli, enveloppes, LFO, envois, table et TIC |
| **Table** | `LFO` | 16 lignes : TRANSP, VOL, FX1–FX3 ; TIC en en-tête |
| **Groove** | `GLO` | 16 entrées de ticks, avec la durée totale de la phrase en ticks. Un deuxième appui ouvre la vue **Projet** : tempo, nom, emplacements, nouveau, charger, sauvegarder |
| **Mixer** | `FX` | Volume et panoramique des 8 pistes, mute/solo, vumètres. Un deuxième appui ouvre **Effets** : chorus, delay, réverbe, tape, crush, limiteur |

**Vue Phrase (P1-05) :** en-tête `PHRASE 00`, `T1`, ▶ (vert, en lecture) ou ■, tempo, `OCT3`, mini-carte ; une ligne de 12 px par pas, numéro du pas en hexa (en couleur tous les 4 pas) ; cellule vide en `--`/`---` gris ; curseur sur fond ambre ; ligne jouée sur fond bleu nuit ; pied : `CUT RES SDL SRV` de l'instrument, puis le nom de la colonne et sa valeur. Le choix de la face reste ouvert (Q-02).

La **mini-carte** en haut à droite montre la position dans la chaîne de vues `Song → Chain → Phrase → Instr → Table`, pour naviguer en profondeur comme sur le M8.

## 4. Les contrôles du FM-1

### 4.1 Les encodeurs et le potentiomètre

| Contrôle | Seul | Avec `SEL` maintenu |
| --- | --- | --- |
| `SELECT` | Curseur haut/bas (ligne) | Objet précédent/suivant du même type (phrase 1A → 1B) |
| `ALGORITHM` | Curseur gauche/droite (colonne) | **Naviguer en profondeur** : à droite, on entre dans l'objet sous le curseur (cellule de song → chain → phrase → instrument → table) ; à gauche, on remonte |
| `PRESETS` | Valeur sous le curseur ±1 (une note : ±1 demi-ton ; une commande FX : défile la liste) | ±0x10 (une note : ±1 octave) |
| `KNOB1–4` | Les 4 cases du pied **[PROTO]**. En Song, Chain et Phrase : CUT, RES, envoi delay, envoi réverbe de l'instrument de la piste en cours, en direct. Avec REC, le geste s'écrit en FX dans le pas joué. Dans les autres vues : les 4 champs de la ligne sous le curseur | — |
| `MASTER` | Volume général | — |

### 4.2 Les boutons

| Bouton | Seul | Avec `SEL` maintenu |
| --- | --- | --- |
| `SEL` | Modificateur, comme SHIFT | — |
| `EDIT` | Sur une cellule vide : insère la dernière valeur. Sur une note : `OFF` (sur `OFF`, la note revient). Sur une autre valeur : elle devient la dernière valeur. Sur un n° de chain ou de phrase vide, deux appuis prennent le **premier numéro libre** | **Efface** la cellule |
| `PLAY` | Lire/arrêter selon la vue (§2) | Lire la song à partir de la ligne en cours |
| `REC` | Enregistrement en direct : les touches jouées s'écrivent au pas le plus proche, et les potards en FX | — |
| `OCT−` / `OCT+` | Octave du clavier (0–8) | Pas d'édition : de combien de lignes le curseur avance après une note (0–F) |
| `HOME` `ARP` `SEQ` `ENV` `LFO` `GLO` `FX` | Aller directement à la vue (§3) | — |
| `SAVE` | Sauvegarder le projet | — |
| `SAVE` maintenu 1 s | Menu Projet | — |

### 4.3 Le clavier (27 touches : 16 blanches, 11 noires)

Le rôle des touches dépend de la colonne sous le curseur. C'est **le grand atout de Trawler face au M8**.

| Colonne sous le curseur | Les touches seules |
| --- | --- |
| NOTE | Les 27 touches jouent la note, l'écrivent (avec l'instrument courant si le pas n'en a pas), et le curseur descend du pas d'édition. On entend la note pendant la saisie |
| Valeur en hexa (VOL, INSTR, valeurs FX, n° de chain ou de phrase, paramètres) | **Les 16 touches blanches valent 0 à F**, de F3 à G5, et s'allument faiblement. On tape deux chiffres : le premier s'affiche `7_`, le second complète la valeur ; un chiffre seul est validé après 1,5 s ou au geste suivant **[PROTO]** |
| Commande FX | Les touches blanches 1 à 6 choisissent une famille (hauteur, temps, structure, son, envois, global) et s'allument faiblement ; un nouvel appui passe à la commande suivante de la famille ; PRESETS fait défiler, SEL+PRESETS saute de famille |

**`SEL` + une touche, quelle que soit la colonne :**

- **`SEL` + une touche blanche de 1 à 8** (F3 à C4) : aller à la piste 1 à 8. En Chain et en Phrase, la vue montre ce que joue cette piste à la ligne de song en cours.
- **`SEL` + une touche noire** : la palette d'actions, rappelée sur l'écran d'aide.

  | Touche | Action | Touche | Action |
  | --- | --- | --- | --- |
  | F#3 | Copier | G#4 | Mode sélection (on étend avec SELECT/ALGORITHM) |
  | G#3 | Couper | A#4 | Annuler |
  | A#3 | Coller | C#5 | Rétablir |
  | C#4 | Cloner (copie complète sous un nouveau numéro) | D#5 | Mute de la piste en cours |
  | D#4 | Effacer | F#5 | Solo de la piste en cours |
  | F#4 | Insérer une ligne (décale vers le bas) | | |

### 4.4 Les LED

| LED | Allumée quand |
| --- | --- |
| Touches blanches | Lecture : **la position du pas** sur 16 touches. Colonne hexa : les 16 touches, en clavier |
| Touches noires | `SEL` maintenu : les actions possibles |
| Boutons | Le bouton de la vue en cours est allumé ; PLAY quand ça joue ; REC en enregistrement |

### 4.5 Le clavier de l'ordinateur (FM-1 virtuel, `web/emu/keymap.js`)

Les touches sont prises **par position** (`KeyboardEvent.code`), données ici en QWERTY : sur un AZERTY, Z est la touche marquée W, Q celle marquée A, M celle marquée « , ». Seule la légende change de disposition (QWERTY, AZERTY Mac, AZERTY PC, au choix sur la page).

| Touche d'ordinateur (position QWERTY) | Contrôle du FM-1 |
| --- | --- |
| Rangée Z X C V B N M, et S D F H J au-dessus | F3 à E4 : un piano à partir de F, comme les touches du FM-1 (Z = F3) |
| Rangée Q W E R T Y U I O, et 2 3 4 6 7 9 au-dessus | F4 à G5 |
| ↑ ↓ | SELECT −/+ |
| ← → | ALGORITHM −/+ |
| `[` `]` | PRESETS −/+ |
| ⌥1 ⌥2 … ⌥7 ⌥8 (Alt sur PC) | KNOB1 −/+ … KNOB4 −/+ (ADR-017) |
| Maj | SEL |
| Entrée | EDIT |
| Espace | PLAY |
| `` ` `` (ou la touche à gauche de 1 / à côté de Maj gauche selon le clavier) | REC |
| `-` `=` | OCT− / OCT+ |
| F1–F7 (avec `fn` sur Mac) | HOME, ARP, SEQ, ENV, LFO, GLO, FX |
| Ctrl+S | SAVE |

Les potards et les flèches suivent la répétition automatique du clavier ; les autres touches non. Quand la fenêtre perd le focus, tout ce qui était maintenu est relâché. Le Web MIDI en entrée joue les notes comme les 27 touches.

**[PROTO]** Taper directement 0–9 et A–F sur le clavier de l'ordinateur dans les colonnes hexa serait plus naturel, mais ces touches servent déjà au clavier musical et aux potards. Deux options à tester en P1 : un mode « Verr. hexa », ou des potards à la molette uniquement. Cette correspondance de touches n'existe que dans `keymap.js` et ne touche pas au firmware.

## 5. Les instruments

Paramètres communs à tous les types :

| Groupe | Paramètres |
| --- | --- |
| Base | `TRN` transposition, `FIN` réglage fin, `VOL`, `PAN` |
| Filtre | `FLT` (LP/HP/BP/OFF), `CUT`, `RES` |
| Ampli | `DRV`, enveloppe d'ampli `ATK DEC SUS REL` |
| Enveloppe 2 | `ATK DEC` + destination + quantité |
| LFO | Forme, `RAT`, destination, quantité |
| Envois | `SCH` chorus, `SDL` delay, `SRV` réverbe |
| Table | `TBL`, `TIC` |

| Type | Moteur | Paramètres propres | Pour |
| --- | --- | --- | --- |
| `WAV` | Oscillateur VA ou wavetable | `SHP` (scie, carré, pulse, triangle, sinus, bruit, 10 tables d'onde), `PW` ou `MRP` (forme d'onde ou morphing), `VOX` 1–3 oscillateurs décalés (supersaw léger), `DET`, `SUB` | Pads, leads et basses synthwave |
| `FM4` | FM à 4 opérateurs, 8 algorithmes | `ALG`, `FB`, et pour chaque opérateur `RAT` (ratio), `LVL`, `DEC` | Rhodes et cloches lofi, basses FM |
| `DRM` | Batterie synthétique | `MDL` (KICK, SNARE, HAT, CLAP, TOM, PERC), `TUN`, `DEC`, `TON`, `SNP` (bruit), `DRV`. La note décale la hauteur | Les rythmes |

Plus tard : `SMP` (sampler), `MID` (MIDI Out), `FM6` (portage de msfa/Dexed, sous Apache-2.0, comme Felucca).

### Les octets d'un instrument (ADR-025)

`instr_t.p[]` (53 octets) : les paramètres communs d'abord, identiques pour tous les types, puis ceux du moteur à partir de l'octet 24 (`tracker/instr.h`). Les projets sauvegardés stockent ces positions : un nouveau paramètre prend un octet libre de son groupe, jamais entre deux existants.

| Octets | Paramètres |
| --- | --- |
| 0–3 | `TRN` (demi-tons, signé), `FIN` (1/128 de demi-ton, signé ; ±1 demi-ton), `VOL` (v/0x80), `PAN` (80 au centre) |
| 4–7 | `FLT` (00 OFF, 01 LP, 02 HP, 03 BP), `CUT`, `RES`, `DRV` |
| 8–11 | `ATK`, `DEC`, `SUS`, `REL` |
| 12–15 | Enveloppe 2 : `ATK`, `DEC`, destination, quantité |
| 16–19 | LFO : forme, `RAT`, destination, quantité |
| 20–22 | `SCH`, `SDL`, `SRV` |
| 24– | Le moteur. `WAV` : `SHP` (00 scie, 01 carré, 02 pulse, 03 triangle, 04 sinus, 05 bruit, 06–0F tables d'onde), `PW`, `VOX`, `DET`, `SUB` |

Types : `00 WAV`, `01 FM4`, `02 DRM`, `FF` aucun. Un instrument sans moteur (ou d'un type pas encore écrit) ne sonne pas : la note relâche la voix. Choisir un type remet tous ses paramètres aux valeurs par défaut (`instr_set_type`).

### Le moteur `WAV` (P1-03)

- **Oscillateurs :** scie et carré anticrénelés par polyBLEP (Välimäki et Huovilainen, 2007), en point fixe ; triangle et sinus calculés directement (le coin du triangle crénèle 40 dB sous une scie brute). Sous 12 kHz, le repli d'une scie à 3 520 Hz descend de −21 à −46 dB. Pulse, bruit et tables d'onde : P3-07 (d'ici là, pulse joue le carré).
- **Filtre :** SVF « trapézoïdal » d'Andrew Simper (Cytomic), stable à toutes les coupures et résonances. `CUT` 00–FF va de 30 Hz à 18 kHz, en exponentielle. `RES` FF donne un gain d'environ 43 à la résonance ; le signal est écrêté à la pleine échelle après le filtre.
- **Enveloppe :** `ATK` linéaire ; `DEC` et `REL` exponentiels (−60 dB dans le temps indiqué) ; temps de 1 ms (00) à 10 s (FF), en exponentielle (80 ≈ 100 ms). Fin à −72 dB, puis silence exact.
- **VOL** : v/0x80, multiplié par le VOL du pas (FF = 1). **PAN** : balance ; le côté opposé baisse, le côté proche reste à 0 dB (80 = les deux à 0 dB).
- Défauts : scie, LP, `CUT` C0, `RES` 20, `ATK` 00, `DEC` 80, `SUS` C0, `REL` 70, `VOL` 80, `PAN` 80.

## 6. Les commandes FX (MVP)

Chaque commande s'écrit en 3 lettres suivies de 2 chiffres hexa. Les codes sont un `enum` dans `fx.h`, avec une table de noms. **On n'ajoute de nouveaux codes qu'à la fin de l'enum** : les projets sauvegardés stockent les numéros.

| Catégorie | Code | Paramètre | Effet |
| --- | --- | --- | --- |
| Hauteur | `ARP` | xy | Arpège note, +x, +y demi-tons, un cran par tick |
| | `ARC` | xy | Mode d'arpège x (montant, descendant, aller-retour, aléatoire), vitesse y ticks |
| | `PSL` | xx | Glisse vers la note en xx ticks |
| | `PBN` | xx | Pitch bend continu, signé (01–7F monte, 80–FF descend) |
| | `VIB` | xy | Vibrato : vitesse x, profondeur y |
| | `PIT` | xx | Décalage de hauteur signé, en demi-tons |
| | `FIN` | xx | Réglage fin signé |
| Temps et hasard | `DEL` | xx | Retarde le pas de xx ticks |
| | `KIL` | xx | Coupe la note après xx ticks |
| | `RET` | xy | Redéclenche toutes les y ticks, le volume variant de x (signé sur 4 bits) |
| | `CHA` | xx | Probabilité que le pas joue (xx/FF) |
| | `NTH` | xy | Joue seulement au tour y d'un cycle de x tours |
| | `RND` | xx | Ajoute un aléa de 0 à xx à la valeur de la commande placée à gauche |
| | `HOP` | xx | Saute au pas ou à la ligne xx |
| Structure | `TBL` | xx | Change la table de l'instrument |
| | `TIC` | xx | Vitesse de la table |
| | `THO` | xx | Saut dans la table |
| | `GRV` | xx | Groove de la piste |
| | `TPO` | xx | Tempo en BPM (28–FF ; au-delà de 255, on le règle dans la vue Projet) |
| Son | `VOL` | xx | Décalage de volume signé |
| | `CUT` | xx | Fréquence de coupure |
| | `RES` | xx | Résonance |
| | `FLT` | xx | Type de filtre |
| | `PAN` | xx | Panoramique |
| | `DRV` | xx | Saturation |
| Envois | `SCH` | xx | Envoi chorus |
| | `SDL` | xx | Envoi delay |
| | `SRV` | xx | Envoi réverbe |
| Global | `MDT` | xx | Temps du delay, en pas |
| | `MDF` | xx | Réinjection du delay |
| | `MRS` | xx | Taille de la réverbe |
| | `MCR` | xx | Bitcrush du master |
| | `MTP` | xx | Saturation tape du master |

Ce vocabulaire de 3 lettres est commun aux trackers. **On ne reprend pas les textes d'aide du M8 :** l'aide intégrée de Trawler est rédigée de zéro.

## 7. Le mixer et les effets (budget mémoire dans le pool)

| Bloc | Réglages | Mémoire |
| --- | --- | --- |
| Chorus stéréo | Vitesse, profondeur | ~4 Kio |
| Delay mono vers stéréo | Temps synchronisé, de 1/16 à 3/4 de mesure, au plus 0,75 s en 16 bits ; réinjection ; passe-haut | ~66 Kio |
| Réverbe | Freeverb allégé ; taille, amortissement | ~32 Kio |
| Master | Volume, saturation tape, bitcrush (bits et fréquence), limiteur | — |
| **Total des effets** | | **~102 Kio sur 336** |

Le projet (~34 Kio) et les tables d'onde générées (~16 Kio) tiennent dans le reste.

## 8. La sauvegarde et le format `.trw`

- **En-tête :** `"TRW1"`, version du format en `uint16`, taille, CRC32. Viennent ensuite des blocs TLV : `SONG`, `CHNS`, `PHRS`, `INST`, `TABL`, `GROV`, `MIXR`, `PROJ`.
- On écrit seulement les objets non vides : un n° sur 1 octet, puis les données.
- On peut ajouter des champs ; les anciens projets restent lisibles.
- La sérialisation et la désérialisation sont du C commun au web et à l'appareil (`tracker/serial.c`), avec un test aller-retour.
- **Dans le navigateur :** sauvegarde automatique dans IndexedDB toutes les 5 s si le projet a changé, et 8 emplacements. La page, en dehors du firmware, propose aussi export et import `.trw`, export WAV, et chargement des démos.
- **Sur l'appareil (P5) :** emplacements en flash, écrits seulement à l'arrêt, en double copie avec CRC (comme zp12).

## 9. Le démarrage

1. Au premier lancement, la démo synthwave se charge et la vue Song s'affiche, curseur en 00. Il suffit d'appuyer sur PLAY : le son vient tout de suite.
2. L'aide en surimpression s'ouvre en maintenant **SEL + HOME**. Elle montre une carte des contrôles de la vue en cours, rendue par le firmware lui-même, donc identique sur le web et sur l'appareil.
