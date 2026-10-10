# 06 — Backlog

## Comment le lire

- Chaque phase est une **tranche verticale** : elle se termine sur quelque chose de jouable dans le navigateur.
- Format d'une story : `ID — titre`, puis le besoin, puis les **critères d'acceptation (CA)**.
- **Prérequis de chaque story :** `make lint test` au vert, et un commit dont le message porte l'ID.
- Taille : **S** ≈ une session Claude Code, **M** ≈ 2–3 sessions, **L** = à découper avant de commencer.
- Statut : ☐ à faire · ◐ en cours · ☑ fait. Claude Code met à jour la case.

---

## P0 — Squelette : « un bip dans le FM-1 virtuel »

**☑ P0-01 — Initialiser le repo (S)**

Le repo prend forme : git, LICENSE GPL-3.0, LICENSING.md, .gitignore (`build/`, `vendor-ref/`), Makefile avec les cibles `host web serve test lint`, dossiers de l'archi §2.

- CA : `make lint` passe sur un repo vide.
- CA : `vendor-ref/` contient les 7 clones de référence (CLAUDE.md).

**☑ P0-02 — Écrire `plat.h` et le point fixe (S)**

`plat.h` reprend l'archi §3. `dsp/fixed.h` fournit Q15/Q12, `mul_q15`, `sat16` et une phase sur 32 bits. `tools/gen_tables.py` génère un sinus de 1024 points et la table note → incrément de phase.

- CA : tests unitaires de `fixed.h` (saturation, arrondis).
- CA : les tables sont générées dans `build/gen/`.

**☑ P0-03 — Simulateur hôte et scénarios (M)**

`host/trawler_host.c` implémente `plat.h` : framebuffer, masques des boutons et touches maintenus, accumulateurs d'encodeurs, stockage en mémoire, file MIDI, horloge `run_ms()` (blocs de 256, `app_frame` toutes les 16 ms). Il lit les scénarios `.trs` et écrit des WAV et des PNG.

- CA : `tests/scenarios/hello.trs` (appuyer sur une touche, attendre, `wav`, `shot`) produit un WAV non vide et un PNG de 240×240.

**☑ P0-04 — App « hello » (S)**

Une app minimale :
- un sinus monophonique joué au clavier (27 touches, octave par OCT±) ;
- l'écran affiche la note et le nom de l'app ;
- les LED des touches s'allument.

- CA : golden WAV et PNG enregistrés.

**☑ P0-05 — Build wasm et worklet (M)**

`trawler_web.c`, `worklet.js` et `index.html`, selon l'archi §4–5. Emsdk figé dans Docker.

- CA : `make web serve`, puis un clic sur Démarrer : la touche Z du clavier d'ordinateur joue un sinus, et l'écran du FM-1 apparaît en ×2 dans un canvas.
- CA : `emu_test.mjs` confirme que le wasm et le natif rendent les mêmes octets sur le scénario hello.

**☑ P0-06 — Panneau SVG cliquable (M)**

Une silhouette du FM-1 avec ses 27 touches (et les légendes gravées des touches noires, voir `01_FM1_HARDWARE.md`), ses 14 boutons étiquetés, ses 7 encodeurs et le MASTER. Zones de clic en `div`, molette et glisser sur les potards, clic droit ou appui long pour verrouiller (ADR-018), LED reflétées, mise en page desktop et mobile.

- CA : chaque contrôle visible agit sur le simulateur.
- CA : utilisable au doigt sur iPad.
- CA : la légende du clavier d'ordinateur a un sélecteur QWERTY / AZERTY (Mac et PC), qui ne change que l'affichage : les touches restent associées à leur position physique (`KeyboardEvent.code`). Safari et Firefox ne savent pas lire la disposition ; sur Chrome et Edge, `navigator.keyboard.getLayoutMap()` peut choisir la disposition par défaut. Q-17 tranchée : ADR-017.

**☑ P0-07 — CI et GitHub Pages (S)**

GitHub Actions lance `make lint test web` et déploie `build/emu` sur GitHub Pages.

- CA : l'URL publique joue le hello.

---

## P1 — Cœur du tracker : « je tape une phrase et je l'entends »

**☑ P1-01 — Modèle de données (S)**

`model.h` et `model.c` (spec §1) : initialisation à vide, accesseurs, constantes dans `config.h`.

- CA : `size_report.py` affiche la taille du projet en mémoire, qui doit rester sous 48 Kio.

**☑ P1-02 — Horloge du séquenceur et lecture d'une phrase (M)**

Spec §2 : Q8 exact à l'échantillon, découpe du bloc aux ticks, groove 0 (06/06), lecture d'une phrase en boucle, une voix par piste avec fondu de 2 ms.

- CA : test sans dérive, l'écart cumulé sur 10 min doit rester sous 1 échantillon.
- CA : golden WAV d'une phrase de 4 notes à 120 BPM.

**☑ P1-03 — Moteur `WAV`, première version (M)**

Scie, carré et triangle anticrénelés (polyBLEP en point fixe), filtre SVF LP/HP/BP, enveloppe ADSR, `VOL`, `PAN`. Coût déclaré (archi §7).

- CA : golden WAV de chaque forme d'onde.
- CA : aucun débordement à résonance maximale (test de saturation).

**☑ P1-04 — Police et primitives graphiques (S)**

`gfx.c` (rectangles, texte, couleurs du thème) et `font.c` généré par `gen_font.py`. **[PROTO]** On produit deux maquettes de la vue Phrase, en 6×10 et en 8×12, et Anthony choisit.

- CA : deux PNG de comparaison produits dans `docs/mockups/` (`make mockups`). Choix provisoire : 8×12, à confirmer (Q-02).

**☐ P1-05 — Vue Phrase : affichage et curseur (M)**

Grille de 16 pas × 6 colonnes, en-tête, pied avec les 4 cases, curseur déplacé par SELECT et ALGORITHM, playhead surligné, LED de position sur les touches blanches. Redessiner seulement les zones qui changent.

- CA : scénario « déplacer le curseur » avec golden PNG.

**☐ P1-06 — Saisie des notes et pavé hexa (M)**

Spec §4.3 :
- les touches entrent les notes (pré-écoute, pas d'édition) ;
- PRESETS ±1, SEL+PRESETS ±16 ;
- les touches blanches servent de pavé hexa à deux chiffres, avec un délai de validation ;
- EDIT insère la dernière valeur, SEL+EDIT efface ;
- `OFF` disponible.

**[PROTO]** Anthony teste la saisie hexa sur les touches et le délai d'1,5 s.

- CA : scénarios pour chaque mode de saisie.

**☐ P1-07 — PLAY, tempo et vue Projet minimale (S)**

PLAY lit la phrase en boucle et l'arrête. Le tempo se règle dans la vue Projet (GLO ×2).

- CA : on entend la phrase tapée dans le FM-1 virtuel. **🎉 Jalon P1.**

---

## P2 — Structure : « une vraie chanson à 8 pistes »

**☐ P2-01 — Vue Chain (S)**

Colonnes PHRASE et TRANSP. Deux appuis sur EDIT prennent la prochaine phrase libre. SEL+ALGORITHM → entre dans la phrase.

**☐ P2-02 — Vue Song et lecture de la song (M)**

8 pistes, lecture indépendante, retour au début du bloc sur une cellule vide (spec §2), SEL+PLAY.

- CA : golden WAV d'une song à 3 pistes de longueurs différentes.

**☐ P2-03 — Navigation entre les niveaux et mini-carte (S)**

SEL+ALGORITHM pour descendre ou remonter, SEL+SELECT pour l'objet suivant, SEL + touche 1–8 pour changer de piste, boutons d'accès direct, mini-carte.

**☐ P2-04 — Vue Instrument (M)**

Liste des paramètres du type `WAV`, édition par PRESETS et par les potards, choix du nom, table par défaut.

- CA : changer le type d'un instrument remet ses paramètres par défaut, après confirmation.

**☐ P2-05 — Grooves (S)**

Vue Groove et commande `GRV`.

- CA : un groove 08/04 produit le swing attendu (test en ticks).

**☐ P2-06 — Édition : sélection, copier/couper/coller, cloner, annuler (M)**

Palette SEL + touche noire (spec §4.3). Annulation sur 16 niveaux, par instantané différentiel.

- CA : tests unitaires de `edit.c`.

**☐ P2-07 — Mixer, mute et solo (S)**

Volume et panoramique par piste, vumètres, mute et solo.

**☐ P2-08 — Sérialisation `.trw` et sauvegarde dans le navigateur (M)**

`serial.c` (spec §8), sauvegarde automatique dans IndexedDB, 8 emplacements, SAVE et menu Projet. La page propose l'export et l'import d'un `.trw`.

- CA : test aller-retour octet par octet.
- CA : un rechargement de la page ne perd rien. **🎉 Jalon P2.**

---

## P3 — Expression : « ça sonne comme un vrai morceau »

**☐ P3-01 — Moteur des commandes FX et commandes de hauteur (M)** — `fx.c`, avec ARP, ARC, PSL, PBN, VIB, PIT, FIN. Sélecteur de commande par catégorie.

**☐ P3-02 — Commandes de temps et de hasard (M)** — DEL, KIL, RET, CHA, NTH, RND, HOP, avec un générateur aléatoire à graine fixe, pour que le rendu reste reproductible.

**☐ P3-03 — Tables (M)** — Vue Table, réglage TIC, commandes TBL, TIC et THO. Une table jouée par chaque instrument.

- CA : golden WAV d'un arpège joué par une table.

**☐ P3-04 — Commandes de son et d'envoi (S)** — VOL, CUT, RES, FLT, PAN, DRV, SCH, SDL, SRV, TPO, GRV.

**☐ P3-05 — Effets d'envoi (M)** — Chorus, delay synchronisé, réverbe, avec les budgets de la spec §7.

**☐ P3-06 — Bus master (S)** — Saturation tape, bitcrush, limiteur. Commandes MDT, MDF, MRS, MCR, MTP.

**☐ P3-07 — `WAV` complet (M)** — PW, tables d'onde avec morphing, VOX 1–3, DET, SUB, enveloppe 2, LFO.

**☐ P3-08 — Moteur `FM4` (M)** — 4 opérateurs, 8 algorithmes, réinjection, décroissance par opérateur.

**☐ P3-09 — Moteur `DRM` (M)** — KICK, SNARE, HAT, CLAP, TOM, PERC.

- CA : la jauge CPU estimée reste ≤ 50 % avec 8 pistes `FM4` et tous les effets. **🎉 Jalon P3.**

---

## P4 — Finition et partage : « je le montre à quelqu'un »

**☐ P4-01 — Aide en surimpression (S)** — SEL+HOME affiche la carte des contrôles de la vue en cours, rendue par le firmware.

**☐ P4-02 — Enregistrement en direct REC (M)** — Les notes s'écrivent au pas le plus proche, les gestes des potards en FX (spec §4.1).

**☐ P4-03 — Export WAV (S)** — Rendu de la song plus vite que le temps réel dans le worklet ou un Worker, puis téléchargement.

**☐ P4-04 — Projets de démo (M)** — Deux projets, synthwave (≈ 100 BPM, groove droit, supersaw, basse FM4, batterie DRM, réverbe longue) et lofi (≈ 80 BPM, swing 08/04, Rhodes FM4, crush et tape). Anthony les compose avec Claude, par script `make_demo.py` ou à la main.

- CA : la démo synthwave se charge au premier lancement.

**☐ P4-05 — Checklist des navigateurs et accessibilité (S)** — Chrome, Edge, Firefox, Safari et iPad. Repli si 44,1 kHz est refusé. Focus clavier, contraste.

**☐ P4-06 — Manuel utilisateur (S)** — `docs/MANUEL.md` et une page sur le site, écrits de zéro.

- CA : le test « premier son en moins de 5 min » est passé par une personne extérieure. **🎉 Jalon MVP.**

---

## P5 — Sur un vrai FM-1 (il faut un appareil)

**☐ P5-01 — Toolchain JieLi en Docker** (copier depuis Felucca), qui compile un Dinghy de test.

**☐ P5-02 — `plat_fm1.c`**, au-dessus de la couche matérielle de Felucca, avec le mode secours.

**☐ P5-03 — Mesure du CPU réel**, recalibrage du modèle de coût, optimisation si besoin.

**☐ P5-04 — Stockage en flash** : écritures seulement à l'arrêt, double copie avec CRC.

**☐ P5-05 — Paquet `.fwsc`** avec l'identité FM-1_95N (à confirmer) et un nouveau VID/PID, installeur web.

**☐ P5-06 — Release GitHub et inscription sur le hub** (contacter zvenson).

---

## Plus tard (idées)

- Sampler `SMP` en ADPCM IMA, échantillons chargés depuis le navigateur.
- MIDI Out `MID`, Web MIDI en sortie.
- Gammes.
- `FM6` compatible Dexed.
- Thèmes de couleurs.
- Partage d'un projet par URL (base64 compressé).
- Mode « live », où on lance une chain par touche.
