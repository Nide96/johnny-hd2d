# Couverture des scènes — personnage retro v1

Inventaire reproductible : graine 4242, horloge 1993-06-01T10:00, chapitre 1.
Chaque scène est forcée puis exécutée jusqu’à sa fin, sans boucle de démo.
Les tirages aléatoires d’une autre graine peuvent emprunter d’autres branches.
Les comptes sont des commandes de dessin par pas VM, pas des images GPU.

**Archive de correspondances seulement : le renderer actuel n’active aucun personnage RGBA.**

**84 dessins bitmap HD et 2 symboles vectoriels ; 58/79 scènes rencontrent au moins une
dessin remplaçable dans ce parcours.** Cela ne signifie pas que ces scènes sont
entièrement redessinées. Les accessoires et corps partiels sans correspondance restent originaux.
Les colonnes de garde héritée et de compatibilité sont les métadonnées VM mesurées,
pas la sélection du renderer hybride. Celui-ci harmonise aussi les compositions
complètes non décomposables ; voir `VALIDATION-UNIFORME.md`. Les données ne prouvent
pas une couverture GPU exhaustive.

Régénération depuis le dossier projet : `make inventory`, puis
`python tools/report_scene_coverage.py`. Les originaux doivent être présents.

| Scène | Pas VM | Garde VM héritée | Compatibles à ce pas | Dessins HD candidats | Familles enrichies |
|---:|---:|---:|---:|---:|---|
| 1 | 262 | 262 | 262 | 0 | — |
| 2 | 260 | 260 | 260 | 37 | JOHNWALK.BMP |
| 3 | 160 | 160 | 160 | 31 | JOHNWALK.BMP |
| 4 | 729 | 729 | 729 | 19 | JOHNWALK.BMP |
| 5 | 132 | 132 | 132 | 7 | JOHNWALK.BMP |
| 6 | 560 | 560 | 560 | 83 | JOHNWALK.BMP, MJREAD.BMP |
| 7 | 429 | 429 | 429 | 71 | JOHNWALK.BMP, MJREAD.BMP |
| 8 | 326 | 0 | 0 | 128 | JOHNWALK.BMP |
| 9 | 690 | 690 | 690 | 45 | JOHNWALK.BMP |
| 10 | 846 | 846 | 740 | 262 | LITEBULB.BMP, MJREAD.BMP |
| 11 | 1420 | 0 | 0 | 53 | JOHNWALK.BMP |
| 12 | 545 | 545 | 545 | 91 | JOHNWALK.BMP, LITEBULB.BMP |
| 13 | 464 | 0 | 0 | 91 | JOHNWALK.BMP |
| 14 | 316 | 316 | 316 | 38 | JOHNWALK.BMP |
| 15 | 1904 | 1904 | 1904 | 156 | JOHNWALK.BMP |
| 16 | 734 | 734 | 734 | 102 | JOHNWALK.BMP |
| 17 | 892 | 892 | 892 | 190 | JOHNWALK.BMP |
| 18 | 734 | 734 | 734 | 102 | JOHNWALK.BMP |
| 19 | 892 | 892 | 892 | 190 | JOHNWALK.BMP |
| 20 | 459 | 459 | 459 | 109 | JOHNWALK.BMP |
| 21 | 313 | 313 | 313 | 102 | JOHNWALK.BMP |
| 22 | 271 | 0 | 0 | 78 | JOHNWALK.BMP |
| 23 | 488 | 0 | 0 | 71 | JOHNWALK.BMP |
| 24 | 558 | 558 | 558 | 68 | JOHNWALK.BMP |
| 25 | 247 | 247 | 247 | 68 | JOHNWALK.BMP |
| 26 | 263 | 0 | 0 | 46 | JOHNWALK.BMP |
| 27 | 259 | 259 | 259 | 61 | JOHNWALK.BMP |
| 28 | 397 | 397 | 397 | 103 | JOHNWALK.BMP, LITEBULB.BMP |
| 29 | 237 | 237 | 237 | 67 | JOHNWALK.BMP |
| 30 | 788 | 788 | 788 | 33 | JOHNWALK.BMP |
| 31 | 383 | 383 | 383 | 30 | JOHNWALK.BMP |
| 32 | 678 | 678 | 678 | 0 | — |
| 33 | 368 | 368 | 368 | 32 | JOHNWALK.BMP |
| 34 | 322 | 322 | 322 | 356 | JOHNWALK.BMP, LITEBULB.BMP, MJREAD.BMP |
| 35 | 405 | 405 | 405 | 871 | JOHNWALK.BMP, MJREAD.BMP |
| 36 | 529 | 529 | 529 | 340 | JOHNWALK.BMP, MJREAD.BMP |
| 37 | 500 | 500 | 500 | 0 | — |
| 38 | 480 | 480 | 480 | 227 | JOHNWALK.BMP, MJREAD.BMP |
| 39 | 39 | 39 | 39 | 0 | — |
| 40 | 39 | 39 | 39 | 0 | — |
| 41 | 39 | 39 | 39 | 0 | — |
| 42 | 39 | 39 | 39 | 0 | — |
| 43 | 567 | 567 | 567 | 0 | — |
| 44 | 203 | 203 | 203 | 0 | — |
| 45 | 39 | 39 | 39 | 0 | — |
| 46 | 39 | 39 | 39 | 0 | — |
| 47 | 39 | 39 | 39 | 0 | — |
| 48 | 39 | 39 | 39 | 0 | — |
| 49 | 39 | 39 | 39 | 0 | — |
| 50 | 486 | 486 | 486 | 0 | — |
| 51 | 837 | 837 | 837 | 837 | JOHNWALK.BMP, SLEEP.BMP, STNDLAY.BMP |
| 52 | 919 | 919 | 919 | 918 | JOHNWALK.BMP, SLEEP.BMP, STNDLAY.BMP |
| 53 | 848 | 848 | 848 | 848 | JOHNWALK.BMP, SLEEP.BMP, STNDLAY.BMP |
| 54 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 55 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 56 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 57 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 58 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 59 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 60 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 61 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 62 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 63 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 64 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 65 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 66 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 67 | 95 | 95 | 95 | 95 | JOHNWALK.BMP |
| 68 | 527 | 527 | 527 | 68 | JOHNWALK.BMP |
| 69 | 555 | 0 | 0 | 23 | LITEBULB.BMP |
| 70 | 342 | 342 | 342 | 68 | JOHNWALK.BMP |
| 71 | 451 | 0 | 0 | 0 | — |
| 72 | 431 | 0 | 0 | 0 | — |
| 73 | 354 | 0 | 0 | 0 | — |
| 74 | 175 | 175 | 175 | 0 | — |
| 75 | 211 | 211 | 211 | 0 | — |
| 76 | 653 | 653 | 653 | 32 | LITEBULB.BMP |
| 77 | 201 | 201 | 201 | 0 | — |
| 78 | 540 | 540 | 540 | 19 | JOHNWALK.BMP, LITEBULB.BMP |
| 79 | 316 | 316 | 316 | 9 | JOHNWALK.BMP |

## Ressources non remplacées les plus dessinées

Liste brute incluant personnages, fragments, animaux et accessoires : ne pas
transformer automatiquement tous ces dessins en corps entiers de Johnny.

- ZZZZS.BMP : 6962 dessins
- MJFISH1.BMP : 5152 dessins
- FIRE1.BMP : 5136 dessins
- GJDIVE.BMP : 4108 dessins
- MJDIVE.BMP : 4089 dessins
- FIRE.BMP : 3950 dessins
- SMDATE6.BMP : 3888 dessins
- GJBIPLAN.BMP : 3884 dessins
- MJBOTTLE.BMP : 3310 dessins
- MJ_AMB.BMP : 2590 dessins
- MJCOCO.BMP : 2126 dessins
- GJNAT3.BMP : 2062 dessins
- LILIPUTS.BMP : 1482 dessins
- SHIPS.BMP : 1257 dessins
- SSUZY3.BMP : 1207 dessins
