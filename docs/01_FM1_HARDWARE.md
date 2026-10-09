# 01 — Le M-VAVE FM-1 : matériel et chaîne de firmware

Recherche du 9–10 octobre 2026. Chaque fait renvoie à sa source. Les points marqués ⚠️ ne sont pas vérifiés, ou les sources se contredisent.

## 1. L'appareil

- **Ce que c'est :** un synthé FM de poche à environ 60–80 $ sur AliExpress, ~100 € sur Amazon ([test Synth Anatomy](https://synthanatomy.com/2026/07/m-vave-fm-1-review-low-budget-pocket-fm-ynthesizer-with-iconic-sounds.html), [Synthtopia](https://www.synthtopia.com/content/2026/07/13/m-vave-introduces-fm-1-fm-pocket-synthesizer/)).
- **Firmware d'origine (V15) :** FM à 6 opérateurs façon DX7, 32 algorithmes, 12 voix, 128 patchs, import SysEx DX7, multi-effet mono, arpégiateur, séquenceur de 16 pas.

| Élément | Valeur | Source |
| --- | --- | --- |
| Puce | JieLi **AC791N** (famille WL82), cœur pi32v2 | [Dinghy RULES.md](https://raw.githubusercontent.com/zvenson/dinghy/main/docs/RULES.md) |
| FPU | **Aucune** selon Dinghy. ⚠️ FoMni et X0X compilent avec `-mcpu=r3 -mfprev1` et décrivent une FPU **simple précision** (pas de double). Trawler reste en entiers dans tous les cas (ADR-009) | idem, [FoMni tools/build.py](https://github.com/charlesvestal/fm1-fomni/blob/main/tools/build.py) |
| Horloge | Quartz 24 MHz (TIMER4). Fréquence CPU ⚠️ non vérifiée (« 320 MHz double cœur » selon un tiers) | [fm1_time.h](https://raw.githubusercontent.com/hugelton/Felucca/main/firmware/hal/fm1_time.h) |
| SRAM | 512 Kio (0x01C00000–0x01C80000). Utilisable : **RAM ~96 Kio + pool ~336 Kio** + piles 24 Kio | [Dinghy API.md](https://raw.githubusercontent.com/zvenson/dinghy/main/docs/API.md), [Felucca app.ld](https://raw.githubusercontent.com/hugelton/Felucca/main/firmware/app.ld) |
| Flash | 1 Mio SPI NOR, exécution en place. Emplacement pour l'app **~568 Ko** | [fm1_flash.h](https://raw.githubusercontent.com/hugelton/Felucca/main/firmware/hal/fm1_flash.h) |
| Audio | I2S vers un codec externe (modèle ⚠️ inconnu), 24 bits. **44,1 kHz** nominal (44 117,6 Hz en réalité). L'app rend en Q15 | [fm1_audio.h](https://raw.githubusercontent.com/hugelton/Felucca/main/firmware/hal/fm1_audio.h) |
| Bloc audio | 256 frames toutes les 5,8 ms. On vise < 2,9 ms ⚠️ Felucca parle aussi de demi-tampons de 128 | [RULES.md](https://raw.githubusercontent.com/zvenson/dinghy/main/docs/RULES.md) |
| Clavier | **27 touches F3–G5** (16 blanches, 11 noires), sans vélocité, avec LED | [API.md](https://raw.githubusercontent.com/zvenson/dinghy/main/docs/API.md) |
| Légendes gravées | Sur le boîtier, donc **impossibles à changer par le firmware**. Touches noires, de gauche à droite : `OP1` `OP2` `OP3` · `OP4` `OP5` · `OP6` `PIT` `GLO` · `MONO` `POLY` · (F#5, isolée, sans légende). Touches blanches : aucune légende, de petits points entre les groupes d'octave. `PLAY/STOP` s'écrit sur deux lignes. Voir Q-16 | Photo du produit (2026-10-10) |
| Boutons (14) | `OCT−` `OCT+` `FX` `SEL` `ENV` `LFO` `EDIT` `GLO` `HOME` `SAVE` `ARP` `SEQ` `PLAY` `REC`, avec LED | idem |
| Encodeurs (7) | `SELECT` `ALGORITHM` `KNOB1` `KNOB2` `KNOB3` `KNOB4` `PRESETS`. Sans fin, sans bouton-poussoir | idem |
| Potentiomètre | `MASTER` (volume, lu par l'ADC) | idem |
| Écran | ST7789V **240×240 IPS, RGB565**, SPI 12 MHz. Une image complète fait ~115 Ko : il ne faut redessiner que ce qui change | [lcd.c](https://raw.githubusercontent.com/hugelton/Felucca/main/firmware/src/lcd.c) |
| USB-C | MIDI class-compliant et audio UAC1 (16 bits stéréo vers l'ordinateur) | [usb.c](https://raw.githubusercontent.com/hugelton/Felucca/main/firmware/src/usb.c) |
| Autres | Sortie stéréo jack 3,5 mm (le moteur d'origine est mono), MIDI IN en TRS-A, Bluetooth (que Felucca n'active pas), batterie 2000 mAh (~12 h), haut-parleur. **Pas d'entrée micro ni ligne** trouvée | [LICENSING.md Felucca](https://raw.githubusercontent.com/hugelton/Felucca/main/LICENSING.md) |

## 2. Les règles de la puce (ce qui fait perdre du temps)

Résumé de [Dinghy RULES.md](https://raw.githubusercontent.com/zvenson/dinghy/main/docs/RULES.md), à appliquer dès la version navigateur :

- **Pas de `float` ni de `double`.** Sans libgcc, la liaison échoue. On travaille en point fixe : Q15, Q12, phase sur 32 bits.
- **Pas de division 64 bits** (`__udivdi3` n'existe pas). Une division par zéro ne déclenche pas d'erreur, elle donne une valeur fausse.
- **Pas de constante qui ressemble à une adresse de la ROM** (0xFFC00000–0xFFCFFFFF). Par exemple, `-32767 << 7` vaut 0xFFC00080 et fait échouer le build.
- **Accès aux registres matériels uniquement dans `hal/`.**
- **Callback audio :** pas de flash, d'écran ni d'attente. Les enveloppes et les LFO se calculent par bloc de 16 ou 32 frames. Les données partagées avec la boucle principale passent par des `volatile` à un seul écrivain.
- **Effacer 4 Kio de flash bloque tout pendant des dizaines de ms**, son compris. On sauvegarde donc pendant le silence (à l'arrêt).
- **Chien de garde à 8 s.** Le crash affiche un écran rouge avec le PC fautif.

## 3. Chaîne de build (pour la phase 5, sur l'appareil)

- **Langage et compilateur :** C99 et un peu d'assembleur, en build « unity » (un seul fichier qui inclut tous les modules). Compilateur JieLi clang 4.0.1, cible `pi32v2`, en `-Os`. Il ne tourne que sous Linux x86-64 : sur Mac, il faut un Docker `linux/amd64` ([BUILDING.md](https://raw.githubusercontent.com/hugelton/Felucca/main/BUILDING.md)).
- **Fichiers du SDK :** trois fichiers seulement, tirés du SDK JieLi AC79 v1.2.1 (`uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin`, sous Apache-2.0).
- **Paquet `.fwsc` :** un conteneur JieLi « UFW » qui contient `flash.bin` et `ota.bin`, le chargeur de mise à jour ([fm1pkg_make.py](https://raw.githubusercontent.com/hugelton/Felucca/main/tools/fm1pkg_make.py)).
- **Identité produit :** une chaîne du type `FM-1_915`. Plages déjà prises : 90x SLOOP, 91N Felucca, 92x ChoralRoot/Floyd, 93N sloopDX, 97N zp12, 98N un portage de Doom, 99N Dinghy, 800xxxx FoMni/FiMba, 900xxxx X0X ([MAKE-IT-YOURS.md](https://raw.githubusercontent.com/zvenson/dinghy/main/docs/MAKE-IT-YOURS.md)). → Pour Trawler, on proposera **95N** après avoir demandé à zvenson.
- **Installation :** depuis une page web en Chrome ou Edge (Web MIDI SysEx, même protocole que l'outil de M-VAVE), ou avec `tools/fm1_install.py`. Le hub [fm1.designburgapps.com](https://fm1.designburgapps.com/) liste automatiquement chaque release GitHub qui contient un `.fwsc`.
- **Mode secours :** maintenir **OCT−** à l'allumage. Deux crashs au démarrage y mènent aussi tout seuls. Dernier recours : [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter), un RP2040 branché sur l'USB.
- **USB :** l'identifiant 1209:0001 de Felucca est un ID de test pid.codes, « à remplacer avant une release ».

## 4. Carte de la flash (celle de Felucca, pour s'en inspirer)

| Plage | Contenu |
| --- | --- |
| 0x000000–0x004000 | En-tête du paquet |
| 0x004000–0x093000 | Code de l'application |
| 0x097000–0x0E0000 | Données de l'app : projets, échantillons, presets (~292 Kio) |
| 0x0E0000–0x0E5000 | Zone de mise à jour (staging) — **ne pas toucher** |
| 0x0E5000–0x0E7000 | Sauvegarde automatique |
| 0x0FC000–0x0FF000 | Réglages globaux |

Source : [fm1_flash.h](https://raw.githubusercontent.com/hugelton/Felucca/main/firmware/hal/fm1_flash.h).

## 5. Licence

- Felucca, Dinghy, SLOOP, zp12, FoMni et X0X sont en **GPL-3.0-only**. Les morceaux repris d'ailleurs gardent leur licence : msfa/Dexed (Apache-2.0), DaisySP (MIT), Rings (MIT).
- Conséquences pour Trawler :
  - il sera en GPL-3.0-only, avec le source complet publié ;
  - chaque `.fwsc` est accompagné du texte de la licence Apache des fichiers du SDK JieLi ;
  - on n'utilise pas M-VAVE ou FM-1 comme nom de produit.
