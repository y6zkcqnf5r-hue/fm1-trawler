# 07 — Lancer Claude Code : les prompts

À coller dans l'ordre, depuis ce dossier (`cd ~/"Music/_Vibe Coding/FM1" && claude`).

**Le rythme :** une story par session. Passe en **mode plan** (Shift+Tab) pour toute story de taille M, relis le plan, puis valide. Le reste du temps, laisse Claude Code travailler.

---

### 0. Prise en main (une seule fois)

```
Lis CLAUDE.md puis tous les fichiers de docs/ dans l'ordre. Ne code rien encore.
Clone en lecture seule dans vendor-ref/ les 7 repos listés dans CLAUDE.md (git clone --depth 1).
Ensuite :
1) Résume-moi en 10 lignes ce que tu as compris du produit et des contraintes.
2) Ouvre vendor-ref/fm1-fomni et vérifie que plat.h, host/omni_host.c et web/emu/ correspondent
   à ce que décrit docs/05_ARCHITECTURE.md. Liste les écarts éventuels.
3) Liste tes questions ouvertes sur la spec, classées par phase.
```

### 1. Squelette (P0-01 à P0-03)

```
Mode plan. Implémente P0-01 puis P0-02 du backlog (docs/06_BACKLOG.md).
Inspire-toi de vendor-ref/fm1-fomni pour la structure du Makefile et des en-têtes SPDX.
Arrête-toi quand `make lint test` est vert, coche les stories et propose le message de commit.
```

```
Mode plan. Implémente P0-03 (simulateur hôte + scénarios) en t'inspirant de vendor-ref/fm1-fomni/host/omni_host.c
et de son lanceur de scénarios. Le format .trs doit couvrir : press, release, turn, key, wait, wav, shot, expect.
```

### 2. Premier son dans le navigateur (P0-04 à P0-07)

```
Implémente P0-04 et P0-05. Pour le worklet et le chargement wasm sans glue Emscripten, pars de
vendor-ref/fm1-x0x/web/emu/ et vendor-ref/fm1-fomni/web/emu/. Ajoute le test de parité inspiré de
vendor-ref/ChoralRootFM1/tools/emu/web/test_web_emu.mjs. Montre-moi à la fin la commande pour tester.
```

```
Implémente P0-06 : un panneau SVG maison aux proportions du FM-1 (pas de reprise d'images M-VAVE).
Les 14 boutons portent leur nom réel (OCT−, OCT+, FX, SEL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY, REC).
Puis P0-07 (CI + Pages).
```

### 3. Le cœur du tracker (P1)

```
Mode plan. Implémente P1-01 et P1-02. Avant de coder l'horloge, écris le test « pas de dérive sur 10 min ».
Rappel : pas de division 64 bits, calcul de spt_q8 en 32 bits (spec §2).
```

```
Implémente P1-04 : produis deux maquettes PNG de la vue Phrase (police 6×10 et 8×12) dans docs/mockups/,
remplies avec une phrase d'exemple et 3 colonnes FX. Je choisirai.
```

*(Ensuite P1-03, puis P1-05 à P1-07, une story par session.)*

### 4. Les prompts de routine

- **Reprendre une session :** « Lis CLAUDE.md et docs/06_BACKLOG.md, dis-moi où on en est et quelle est la prochaine story. »
- **Après un test dans le navigateur :** « J'ai testé [ce que j'ai fait]. Ce qui ne va pas : … Propose une correction et mets la spec à jour si c'est une décision de design (docs/08_DECISIONS.md). »
- **Contrôle d'hygiène, de temps en temps :** « Vérifie la conformité aux règles non négociables de CLAUDE.md sur tout firmware/src, et lance size_report. Rapport court. »
- **Quand un budget est dépassé :** « On dépasse [RAM/pool/CPU estimé]. Propose 3 options chiffrées, sans rien appliquer. »

### 5. Conseils

- **Joue après chaque story.** C'est toi le product owner, et tu connais le M8 : ton ressenti dans le FM-1 virtuel compte plus que les tests.
- **Note chaque décision dans `docs/08_DECISIONS.md`**, sinon Claude Code la reperdra à la session suivante.
- **Garde CLAUDE.md court et à jour.** Si une même erreur revient deux fois, ajoute une règle.
- **Commit après chaque story verte.** Le jour où une session part en vrille, `git reset` te ramène au dernier état propre.
