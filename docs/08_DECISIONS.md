# 08 — Décisions (ADR) et questions ouvertes

## Décisions prises

| # | Date | Décision | Pourquoi |
| --- | --- | --- | --- |
| ADR-001 | 2026-10-10 | **On commence par le navigateur**, avec le même code C compilé en wasm et joué dans un AudioWorklet | Pas encore de FM-1. C'est la méthode éprouvée de tout l'écosystème, et le portage tient en un seul fichier |
| ADR-002 | 2026-10-10 | **Code neuf, sur le modèle `plat.h` de FoMni et X0X**, et non un fork de Felucca ou Dinghy | Frontière nette, aucun moteur Felucca à charger en RAM, un tracker est très différent d'un synthé |
| ADR-003 | 2026-10-10 | **Entiers seulement** dans `firmware/src`, budgets de l'appareil appliqués dès le web | La puce n'a pas de FPU. Rien à réécrire au moment du portage |
| ADR-004 | 2026-10-10 | Licence **GPL-3.0-only** | Obligatoire si on reprend la couche matérielle de Felucca, et exigé par le hub |
| ADR-005 | 2026-10-10 | Concept de **tracker inspiré du M8** : Song/Chain/Phrase/Instr/Table/Groove | Personne ne l'a fait sur le FM-1, et Anthony connaît le M8 |
| ADR-006 | 2026-10-10 | **8 pistes monophoniques**, une voix par piste | Comme le M8 ; 8 voix au plus, c'est ce que tient Felucca |
| ADR-007 | 2026-10-10 | Les touches blanches servent de **pavé hexa**, le clavier entre les **notes** | C'est ce qui distingue Trawler du M8 ; à valider en prototype (P1-06) |
| ADR-008 | 2026-10-10 | Emscripten **figé** dans Docker | Le `.wasm` est reproductible et la parité tient |
| ADR-009 | 2026-10-10 | **On garde les entiers seulement**, malgré la FPU simple précision qu'annoncent FoMni et X0X (`-mcpu=r3 -mfprev1`) | Les sources se contredisent (Dinghy : pas de FPU). Les entiers marchent dans les deux cas, et la parité wasm/natif reste exacte sans dépendre de l'arrondi flottant |
| ADR-010 | 2026-10-10 | *(Remplacée par ADR-023 : la constante était fausse.)* **Horloge du séquenceur : quotient et reste.** `spt = 30105600 / bpm` (en Q8), plus un accumulateur du reste `30105600 % bpm`, le tout en 32 bits | Le quotient seul n'est exact que si le BPM divise 2¹³·3·5²·7² (100, 120, 140 oui ; 90 non) : sans le reste, on dérive de ~100 échantillons sur 10 min et le critère de P1-02 échoue |
| ADR-011 | 2026-10-10 | Emscripten **6.0.11** (la version de Hortator), en Docker ou en emsdk local à la même version | Même `.wasm` partout |
| ADR-012 | 2026-10-10 | `app_audio` rend du **Q15 dans un `int32`** (±32767 = pleine échelle), saturé à 16 bits en sortie | Cohérent avec les entiers seulement ; le WAV de test et l'USB audio sont en 16 bits ; marge pour sommer les pistes avant saturation (Q-09) |
| ADR-013 | 2026-10-10 | `plat.h` expose la **LED rouge de PLAY** (`plat_play_red`), allumée pendant REC | Le FM-1 a les deux couleurs, FoMni le fait (Q-10) |
| ADR-014 | 2026-10-10 | Le nom définitif est **Trawler** | Il se range avec les noms de bateaux de l'écosystème (Q-01) |
| ADR-015 | 2026-10-10 | Le 2ᵉ bouton s'appelle **SEL** partout (`B_SEL`, scénarios `.trs`, panneau P0-06), comme il est imprimé sur l'appareil | La photo du produit le confirme. « SCL » est seulement le nom que lui donne le firmware Felucca (et son émulateur) |
| ADR-016 | 2026-10-10 | `make test` construit toujours le wasm et lance la parité wasm/natif (`make parity`). Sans emcc, il échoue avec un message ; `make test PARITY=0` la saute en l'écrivant en clair | La règle 7 est non négociable : un saut silencieux masquerait une régression. Le build wasm prend une seconde |
| ADR-017 | 2026-10-10 | Clavier d'ordinateur : **les chiffres jouent les touches noires** de la rangée haute ; **⌥ (Alt) + 1–8** tourne KNOB1–4 −/+ | Avec les chiffres pris par les potards, F#4…F#5 ne se joueraient plus au clavier, ni SEL + noire (annuler, rétablir, mute, solo). Clôt Q-17 |
| ADR-018 | 2026-10-10 | Panneau : **clic droit (ou Ctrl-clic), ou appui long de 0,5 s au doigt, verrouille un bouton** ; pas Maj-clic | Maj est SEL au clavier : Maj + clic doit rester SEL + le contrôle. L'appui long ne verrouille que si rien d'autre n'a été touché, pour que tenir SEL d'un doigt et taper de l'autre reste momentané |
| ADR-019 | 2026-10-10 | Dépôt public **`y6zkcqnf5r-hue/fm1-trawler`** ; la CI (`.github/workflows/ci.yml`) lance `make lint test web DOCKER=1` à chaque push et PR, et un push sur `main` publie `build/emu` sur GitHub Pages | Le nom suit fm1-fomni et fm1-x0x. `DOCKER=1` en CI couvre l'image figée d'ADR-011 ; le natif y est compilé par gcc, ce qui vérifie aussi que gcc et clang rendent les mêmes octets (même `.wasm` à l'octet près en local, en Docker sur Mac et sous Linux) |

| ADR-020 | 2026-10-10 | **Rapport de taille sur des objets ELF 32 bits** : `make size` compile `firmware/src` avec clang (`--target=thumbv7m-none-eabi -ffreestanding -nostdlibinc -DAPP_DEVICE`) et `tools/size_report.py` lit leurs sections et symboles. Budgets dans `app/config.h` (`BUDGET_*`) | Le binaire hôte ne convient pas : Mach-O sur Mac (pas de taille par symbole) et ELF 64 bits en CI donnent deux chiffres, `APP_POOL` y est vide (pool invisible), et il compte `host/`. Un programme de `sizeof` ne verrait que le projet. Avec `APP_DEVICE`, le pool est une vraie section ; `-nostdlibinc` refuse les en-têtes de la libc, et un `__aeabi_ldivmod` ou un flottant logiciel non résolu fait échouer (règles 1–3). En P5, le script lit l'ELF de l'appareil ; d'ici là, le chiffre du code est indicatif (Thumb-2) |
| ADR-021 | 2026-10-10 | **Modèle : lecture directe, écriture par `model.c` seulement.** Le projet est `static` dans `model.c` (dans le pool) ; les autres modules le lisent par `const project_t *g_proj` ; toute écriture passe par un `*_set` borné, qui finit dans une seule fonction `touch()`. Une colonne = le décalage de son octet dans la struct | L'audio lit vite, sans appel ; écrire sans `model.c` exige un cast, visible en relecture. `touch()` est le crochet de l'annulation (P2-06, ancien octet) et de la sauvegarde automatique (P2-08, `model_rev()`). Le curseur de l'interface, les setters et le journal d'annulation parlent du même numéro de colonne |
| ADR-022 | 2026-10-10 | **`mixer_t` (32 o) et en-tête (16 o)**, détaillés dans la spec §1. Gains sur 8 bits : v/0x80 (80 = 0 dB, FF ≈ +6 dB), panoramique 80 au centre. Mixer : volume et pan par piste, mute et solo en masques, retours chorus/delay/réverbe, réglages des effets, master (gain, tape, crush, limiteur). En-tête : version du format (`uint16`, 1), tempo (`uint16`, 40–300, 120), nom de 12 caractères | Mixer neutre par défaut (tout à 0 dB, au centre). Les envois par piste sont déjà sur l'instrument (spec §5), comme sur le M8 ; le bloc `MIXR` (spec §8) porte donc les réglages globaux des effets. Le tempo dépasse 255 : 16 bits, écrit d'un seul coup pour que l'audio ne lise jamais une moitié |

| ADR-023 | 2026-10-10 | **Horloge en échantillons, sans Q8 :** un tick dure `110250 / bpm` échantillons ; quotient et reste en 32 bits, le reste accumulé ajoute un échantillon quand il atteint `bpm`. Le tick k tombe sur `floor(k·110250/bpm)`. Remplace ADR-010 | ADR-010 simplifiait `44100·60·256/(24·bpm)` en `30105600/bpm` ; c'est **28 224 000/bpm** (30 105 600 donne 980 échantillons par tick à 120 BPM au lieu de 918,75, soit 6,7 % trop lent). Compter directement en échantillons garde l'idée du reste et rend le Q8 inutile : le tempo est entier au MVP (au dixième, ce serait 1 102 500 / (10·bpm)). Vérifié : erreur 0 sur 10 min de 40 à 300 BPM |
| ADR-024 | 2026-10-10 | **Provisoire jusqu'à P1-06 et P1-03 :** l'app hello garde son clavier, et PLAY y joue en boucle une phrase de démo (C-4 D#4 G-4 A#4, OFF au pas E) sur la piste 1, avec une voix sinus (`dsp/voice.c`) | On entend le séquenceur dans le navigateur, et la phrase a un scénario (`phrase.trs`), un golden et la parité wasm/natif. P1-07 jouera la phrase tapée ; P1-03 remplace le sinus par le moteur WAV, et **le golden de `phrase.wav` changera alors** |

## Questions ouvertes

| # | Question | À trancher avant | Piste |
| --- | --- | --- | --- |
| Q-02 | La police : 6×10 ou 8×12 ? | P1-05 | Maquettes de P1-04 |
| Q-03 | Le rôle des KNOB1–4 en Song, Chain et Phrase : réglages du son en direct, ou champs de la ligne ? | P1-06 | Prototype |
| Q-04 | La saisie hexa sur les touches : délai d'1,5 s, ou validation par EDIT ? | P1-06 | Prototype |
| Q-05 | Sortie stéréo ou mono ? | P3-05 | Stéréo prévue (panoramique, chorus) ; à revoir si le CPU manque |
| Q-06 | L'identité produit FM-1_95N est-elle libre ? | P5-05 | Demander à zvenson (hub), voir `MAKE-IT-YOURS.md` de Dinghy |
| Q-07 | Un sampler `SMP` en v1 sur l'appareil ? | Après le MVP | Mémoire libre : ~290 Kio de flash pour les données |
| Q-08 | Quel FM-1 acheter, et quand ? | Avant P5 | Environ 60–100 € (AliExpress, Amazon) |
| Q-09 | `app_audio` sort du Q15 dans un `int32` (archi) ou du 24 bits (FoMni) ? | P0-02 | Proposition : Q15 |
| Q-10 | Ajouter à `plat.h` la LED rouge de PLAY (FoMni `plat_play_red`) pour REC ? | P0-02 | Proposition : oui |
| Q-11 | L'appareil tourne à 44 117,6 Hz (+0,04 %) : on l'accepte ? | P5 | Inaudible ; à mesurer sur l'appareil |
| Q-12 | La zone de données de l'appareil (292 Kio) ne loge que 3 projets de 48 Kio en double copie. 8 emplacements sur le web seulement, ou compression ? | P2-08 | |
| Q-13 | Le budget mémoire des 16 niveaux d'annulation | P2-06 | À chiffrer |
| Q-14 | Le budget CPU de référence, en MAC par frame | P3-09 | D'après les moteurs de Felucca |
| Q-15 | Le stockage sur l'appareil : les objets de Felucca sont limités à 3 840 octets, il faut le nôtre | P5-04 | |
| Q-16 | Les touches noires portent des légendes gravées (`OP1`–`OP6`, `PIT`, `GLO`, `MONO`, `POLY`, voir `01_FM1_HARDWARE.md`). La palette `SEL` + touche noire doit-elle en tenir compte ? | P2-06 | Soit les ignorer (l'aide `SEL + HOME` dit quelle touche fait quoi), soit s'en servir de repère quand le sens colle (`GLO` : glide, `MONO`/`POLY` : une bascule, `PIT` : la hauteur). Le panneau P0-06 affiche ces légendes dans tous les cas, comme sur l'appareil |
