> Jalon historique avant les planches de pêche ; état actuel dans `VALIDATION-PECHE.md`.

# Johnny rétro — validation du 18 octobre 2026

## Direction demandée et intégration

Nouvelle représentation torse nu, short blanc simple, casquette blanche,
barbe foncée conservée, cheveux ébouriffés, petit ventre et allure pataude.
Les sept planches actives ont été réinterprétées, soit les 75 corps entiers et
neuf fragments/accessoires déjà raccordés. Les poses de dos sont aussi torse nu.
La lecture conserve ses corps tronqués, casquette séparée et effets distincts.
Les anciens assets avec chemise crème et short bleu restent comme historique.

Le changement porte sur les PNG, leurs conversions JCR1, leurs coordonnées UV
mesurées et les chemins chargés par main.c. Aucun chemin de marche, délai,
script ADS/TTM, collision, caméra, shader de symbole ou règle d’interpolation
n’a été changé dans ce lot. Les familles sans remplaçant restent originales.

`assets/characters/active-retro.json` est le manifeste actif.
`tools/inspect_retro_sheets.py` génère les JSON et les trois headers de cellules,
puis convertit exactement les pixels RGBA en JCR1. Il vérifie les sept planches,
84 cellules, les gouttières transparentes et l’absence de pixels alpha>=128
coupés. Les PNG ne sont pas retouchés par ce programme. Les anciens outils de
mesure délèguent à celui-ci pour éviter des UV anciens sur une nouvelle texture.
Les prompts exacts sont conservés dans `assets/characters/RETRO-V1.md`.

## Vérifications exécutées

- Inspection visuelle des sept PNG : torse/dos nus et shorts blancs pour les
  corps présents, barbe/casquette conservées, orientations et accessoires revus.
- `make test TESTDIR=../../work/retro-tests` réussi en C99 strict : ressources,
  VM, ordonnanceur, INI, capture, marche, caméra, identités et lecture.
- 79 scènes / 30 537 images classiques reconstruites exactement, sans saturation
  d’atlas ; sept parcours de lecture, 3 571 images, 1 945 dessins MJREAD,
  33 indices rencontrés. Les trois autres restent testés en géométrie seulement.
- Capture de notre vraie fenêtre Vulkan : 152 images sur 19 secondes à 8 images/s,
  arrivée, lecture, symboles et départ. Toutes les poses de Johnny utilisées
  pendant cette capture emploient les nouvelles planches actives.
- Quatre essais Vulkan avec couche de validation, 240 images chacun : marche,
  repos traversant attitudes/assise/sommeil, lecture normale en plein écran,
  lecture UPX en plein écran. Sorties 0 et aucune erreur de validation ; seul
  avertissement connu DPI X11. Les deux premiers emploient SOKOL_DEBUG.
- Scénario normal de 1 000 pas, graine 1, midi : hash inchangé
  `de8bfcfeb48c1c6b`, capture classique exacte et zéro saturation d’atlas.

Les tests ASan/UBSan du jalon précédent restent documentés ; ce lot de données
visuelles a été revérifié avec la suite stricte et les essais graphiques ci-dessus.
L’audit conserve 84 correspondances bitmap et deux symboles vectoriels,
avec un candidat dans 58 parcours sur 79 pour sa graine fixe. Il ne s’agit pas
de 58 scènes entièrement modernisées.

## Portée et limites

Ce dessin vise le costume et l’esprit rétro demandés ; ce n’est pas une
restauration pixel à pixel ni une palette strictement limitée à seize couleurs.
Les proportions/contacts restent une interprétation, avec les mêmes limites
pour les fragments et les appuis que les jalons précédents. Les branches
aléatoires et toutes les interactions ne sont pas revues visuellement.
Le seuil alpha de 0,5 retire les halos translucides éventuels à l’affichage.
La rive, les marées exactes, le radeau, le bloom HDR et l’éclairage des feux
ne sont pas modifiés. Aucun 144 FPS soutenu mesuré.

ELF : 177 784 octets ; UPX : 74 980 octets, images/originaux/bibliothèques en plus.
Staging 64 Mio, neuf textures RGBA actives, comme au jalon précédent.

## Tester

Fermer l’ancienne fenêtre puis relancer :
`./preview-reading-hd.sh 34 --fullscreen`

Repos/sommeil : `./preview-scenes-hd.sh 51 --fullscreen`.
Marche isolée : `./preview-walk-hd.sh --fullscreen`.
Le mode classique conserve les dessins originaux et les previews ne sauvent pas l’INI.
