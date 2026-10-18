# Corrections des symboles et de la marche — 18 octobre 2026

> Historique : la représentation active torse nu/short blanc et ses tailles
> de binaire sont documentées dans `VALIDATION-RETRO.md`.


## Causes et changements

Les points d’interrogation et d’exclamation étaient encore les sprites indexés
LITEBULB 4 et 1 d’origine. En mode moderne avec Johnny HD, ils emploient désormais
un tracé vectoriel dans le shader : rouge uni, liseré ivoire et contour sombre.
Il n’y a pas de dégradé de remplissage ; seule la couverture des bords est
anticrénelée pour le plein écran. Position, taille logique, durée et ordre TTM
restent inchangés. L’ampoule et les deux dessins de personnage de cette ressource
ne sont pas confondus avec les symboles. Le mode classique conserve ses pixels.

La marche de sortie avait deux défauts : une planche v1 dont la perspective et
les proportions variaient, et une interpolation des coins de rectangles qui
changent de largeur/hauteur selon la pose. La planche v2 est orientée à gauche
avec un cycle plus homogène ; son ancien miroir correctif est retiré.
L’interpolation moderne JOHNWALK suit maintenant le pivot de tête et la ligne
des pieds, puis replace la pose courante. Un changement de miroir coupe cette
interpolation. Le flux de pixels classique, les chemins et les délais VM ne
changent pas. La marche demeure dessinée, sans contrainte physique des pieds.

Actifs/précision : `assets/characters/READING-WALK-V2.md`, PNG/JCR v2,
`generated/johnny_reading.h`. La v1 reste conservée comme historique.
La caméra commune et la position du palmier sont conservées.

## Vérifications exécutées

- Suite C99 stricte ASan/UBSan réussie, dont 79 scènes / 30 537 images classiques
  exactes, sans saturation d’atlas, et les sept parcours de lecture.
- Cas concret de sortie 26→27 testé à cinq fractions d’interpolation : le pivot
  se déplace de 404,5 à 401,5, la ligne de sol reste à 289 malgré le rectangle
  qui passe de 40×73 à 32×72. Miroir, coupure de direction et désactivation testés.
- Sélecteur de symboles testé : seulement LITEBULB 1/4 ; pas de remplacement
  accidentel de l’ampoule ou des corps LITEBULB 2/3.
- Nouvelle planche mesurée : six cellules séparées, aucun pixel alpha>=128 coupé.
- Capture de notre fenêtre : 152 images sur 19 secondes à 8 images/s,
  `preview-reading-v2.gif/.mp4/.png`. Revue visuelle du `?`, du `!` et de la
  marche de sortie. Le journal confirme 113 symboles vectoriels soumis et
  l’emploi de la nouvelle planche de marche pendant cette capture.
- Cinq essais Vulkan avec validation, 240 images chacun : symboles en plein
  écran, marche, classique, nuit, UPX en plein écran. Sorties 0, aucune erreur
  de validation, avertissement DPI X11 habituel seulement.
- Scénario normal de 1 000 pas, graine 1, midi : hash `de8bfcfeb48c1c6b`,
  reconstruction classique exacte, zéro saturation d’atlas.

ELF actuel : 177 784 octets ; UPX : 75 012 octets, images/bibliothèques en plus.
Le staging reste de 64 Mio. Aucun débit soutenu de 144 FPS n’a été mesuré.
L’audit rencontre un remplacement candidat dans 58 parcours sur 79 pour sa
graine fixe : 84 dessins bitmap + 2 symboles vectoriels, pas 58 scènes entièrement
modernisées. Les autres familles et les contacts de la variante 35 restent à
traiter comme décrit dans les validations précédentes.

## Tester en plein écran

`./preview-reading-hd.sh 34 --fullscreen`

Directement aux symboles : `./preview-reading-hd.sh 34 --skip-steps 135 --fullscreen`
Directement à la sortie : `./preview-reading-hd.sh 34 --skip-steps 315 --fullscreen`
Quitter puis relancer est nécessaire pour remplacer un ancien processus déjà ouvert.
