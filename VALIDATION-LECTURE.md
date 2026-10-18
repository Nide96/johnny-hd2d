# Validation lecture v1 — 18 octobre 2026

> Historique du lot lecture initial. Les symboles, la marche v2, les tailles
> et les contrôles actuels sont dans `CORRECTIONS-LECTURE.md`.


## Lot intégré

- 36 dessins MJREAD.BMP : 27 corps entiers et 9 fragments/accessoires/effets.
- 6 marches JOHNWALK 24..29 pour l’entrée et la sortie des scènes de lecture.
- Total du projet : 75 poses de corps entiers et 9 fragments/accessoires,
  soit 84 dessins dans sept planches. Décor/palmier en plus.
- Remplacement par identité BMP/index, miroirs, clipping et ordre TTM conservés.
- Petits accessoires : rectangle visible extrait des pixels originaux, puis
  projeté avec les miroirs ; la largeur de stockage ne les grossit pas.
  Aucun pixel original n’est embarqué dans ces métadonnées de géométrie.
- Feuille, casquette, fragment vertical, eau et éclat sans ombre de contact
  artificielle. Les corps entiers conservent une ombre douce de présentation.
- La création du renderer reçoit désormais un tableau de planches, au lieu
  d’ajouter un paramètre distinct pour chaque image. Toutes sont chargées
  une fois ; aucun décodage PNG ni génération en ligne pendant l’exécution.

## Résultats exécutés

`make sanitize TESTDIR=../../work/reading-tests` : succès, C99 strict,
ASan/UBSan, détection des fuites désactivée comme dans les autres jalons.

- Toutes les ressources et les tests VM/ordonnanceur/INI/capture/marche/caméra
  réussissent. **79 scènes / 30 537 images classiques** reconstruites exactement
  à partir des commandes du port C, sans saturation d’atlas.
- `test_character.c` : anciennes garanties d’identité et de durée de vie,
  tables UV des sept planches, 48 poses de base (42 antérieures + 6 marches).
- `test_reading.c` : sept scènes originales rejouées (6,7,10,34,35,36,38),
  **3 571 images, 1 945 dessins MJREAD, 33 indices distincts** pour graine 4242,
  chapitre 1, horloge 2026-10-18T12:00. Les trois autres indices (19,21,22)
  ont des correspondances et des tests de géométrie, sans preuve d’exécution
  dans ces parcours. Les variantes aléatoires ne sont pas exhaustives.
- Les 36 rectangles et leurs quatre combinaisons de miroirs sont comparés
  aux empreintes calculées indépendamment depuis les pixels décodés.
  Les fragments/accessoires sont explicitement distingués des corps entiers.
- Tous les dessins JOHNWALK et MJREAD rencontrés dans la scène 34 ont une
  correspondance HD, y compris l’entrée et le départ obliques.
- Outil de mesure : gouttières transparentes et absence de pixels d’alpha >=128
  coupés pour les 42 cellules nouvelles ; PNG générés laissés intacts.
- Binaires normal et UPX : 1 000 pas de scénario, graine 1, midi,
  hash inchangé `de8bfcfeb48c1c6b`, reconstruction exacte, zéro saturation d’atlas.
- Radeon 780M, couche de validation Vulkan : **huit essais de 240 images**,
  sorties 0, aucune erreur de validation. Lecture/livre, casquette/fragments,
  départ, repos, classique, nuit, release et UPX. Les six premiers utilisent
  le build SOKOL_DEBUG. Les journaux confirment des soumissions des planches
  concernées ; avertissement DPI X11 connu seulement.
- Capture de notre fenêtre stabilisée : scène 34, 152 images sur 19 secondes
  à 8 images/s, avec arrivée, assise, casquette, gestes du livre et départ.
  Variante 35 : 40 images sur 5 secondes pour les fragments et les effets.
  Encodages GIF/MP4 techniques, sans retouche artistique des captures.

L’audit des 79 scènes contient un dessin remplaçable dans 56 parcours, pour
sa graine et son horloge fixes ; ce ne sont pas 56 scènes entièrement modernisées.
Le détail figure dans `assets/scenes/COVERAGE.md`.

## Lancement

`./preview-reading-hd.sh` : scène originale 34 puis reprise de l’histoire.
`./preview-reading-hd.sh 35` : variante casquette/feuille/effets.
`./preview-reading-hd.sh 34 --skip-steps 165` : accès aux gestes du livre.
Les lanceurs injectent une horloge, ne sauvegardent pas l’INI et conservent
le rythme de la VM. L’avance n’est utilisée qu’avant le début de l’aperçu.

## Limites

Les poses sont des interprétations artistiques. La marche générée a une
perspective plus latérale que les originaux et est réfléchie pour respecter
leur direction. Les corps et accessoires s’ajustent à leurs rectangles visibles ;
les proportions et contacts ne sont pas contraints par un squelette. Les jonctions
entre le corps et les fragments de casquette de la variante 35 restent à affiner.
Les autres familles (noix de coco, ampoule, animaux, pêche, etc.) restent originales.
Les tests de géométrie ne prouvent pas une égalité artistique avec 1992.

La validation exhaustive compare les commandes de présentation au canvas du
port C, pas au Win16. La revue visuelle actuelle porte surtout sur 34 et 35.
Les autres scènes, la rive, les marées et toutes les interactions demandent
encore un contrôle visuel. Nuit/scènes incompatibles reviennent au classique.
Pas de mesure soutenue de 144 FPS, ni bloom HDR ou éclairage moderne des feux
ajoutés dans ce lot. La cadence des captures ne mesure pas les FPS du renderer.

ELF : **165 496 octets** ; UPX : **72 704 octets**, hors images, originaux et
bibliothèques. Staging maintenu à **64 Mio** ; neuf textures RGBA au total,
chargées à l’initialisation. Ce chiffre n’est pas la mémoire totale du programme.
