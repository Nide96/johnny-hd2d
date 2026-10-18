# Validation — Johnny original exclusivement (18 octobre 2026)

## Correction des points blancs

La nouvelle passe est ciblée sur le matériau R8 de Johnny :
petits damiers de peau de 2×2 pris en charge, blanc isolé dans l’encre atténué
sans supprimer sa couverture, pas de biseautage entre couleurs de la palette,
rim-light réduit à 15 %. L’anticrénelage des coins conserve la matière fusionnée.
La casquette, le short et les détails utiles des yeux gardent leur lisibilité.

Le test GPU réel observe : éclat isolé RGB121/109/101 ; œil proche de la peau
RGB255/245/235 ; quatre texels du damier court RGB245/166/124 identiques à la
peau continue. Les contrôles de tissu blanc, d’encre et de transparence passent.
Les matériaux génériques conservent leur résultat précédent.

`preview-johnny-sober.png` et son détail montrent exactement STNDLAY.BMP/3,
scène51, chapitre6, pas709, graine4242, horloge17:00, en 1920×1080.
Les deux exécutables de capture figent la VM, distincts du programme livré.
Le décor reste identique ; la différence est confinée à Johnny
(rectangle [906,626]–[1103,715] dans les captures 1920×1080).
Les journaux courants sont `validation/sober-*`. Les captures des sections
suivantes décrivent aussi le jalon de palette précédent.

## Politique active

Le renderer n’accepte plus de chemin de texture du personnage et ne contient
plus de pipeline de remplacement RGBA. Johnny reste en R8 dans les scènes,
la marche de démonstration et avec l’ancien drapeau `--johnny-hd`.
Le décor JCR, le palmier et l’occultation du tronc sont conservés.
Les anciens fichiers de personnages restent archivés sur disque.

## Palette et plein écran

Le damier est d’abord fusionné en lumière linéaire. Une correction de matière
réchauffe ensuite la peau de Johnny ; les contours et la barbe sont brun profond,
les blancs restent lumineux. Cette correction est sélectionnée par ressource,
sans changer la palette du décor ni celle des accessoires autonomes. Les sprites
composites conservent leur palette partagée.

Le test `python3 tools/test_material_gpu.py` compile un harnais temporaire
utilisant le renderer réel puis mesure ses pixels dans sa propre fenêtre.
Sur Radeon 780M / RADV PHOENIX :

- Damier générique : une seule couleur RGB 224/188/185, exactement la moyenne
  linéaire attendue ; transparence préservée.
- Peau Johnny : RGB 245/166/124 ; blancs : 255/245/235 ; barbe : 28/17/11.
- Aucune erreur Vulkan. Le test vérifie aussi le maintien du contraste.

`preview-johnny-original.png` compare deux captures **1920×1080** au même état VM
(scène20, pas101, graine4242, 17:00). La fenêtre est active et son animation de
création est stabilisée avant capture. La différence est confinée à la zone de
Johnny et de sa canne : rectangle [444,537]–[682,726] dans les images 1920×1080.
Le décor est pixel pour pixel identique. Le fichier `preview-johnny-original-detail.png`
est un agrandissement ×2 d’un recadrage de ces captures.

Le viewport GPU utilise la résolution de sortie, sans agrandir une texture JPEG
du personnage ou une cible intermédiaire de 640×480. Les texels R8 sont lus
explicitement ; les diagonales et traits GDI ont un anticrénelage de couverture.
Cela préserve le dessin original, sans inventer de nouveaux détails anatomiques.

## Tests

Compilation C99 avec `-Wall -Wextra -Wpedantic -Werror` réussie.
`make test` et `make sanitize` passent. Les tests de capture reconstruisent
30 537 images sur 79 scènes exactement. Le cœur VM, le canvas logique,
le décompresseur, le moteur ADS/TTM et le RNG n’ont pas été modifiés.

Dix cas de 240 images rendues sur la 780M passent sans erreur de validation :
classique ; pêche R8 ; pêche avec ancien drapeau HD ; construction du radeau ;
lecture ; sommeil ; sommeil nocturne ; marche originale ; composition spéciale ;
binaire compact en plein écran. Les scènes continuent normalement pendant ces
tests, ils ne constituent pas une revue visuelle exhaustive de chaque pose.
Voir `validation/original-summary.json` et les journaux `original-*.log`.

Le staging accepté reste 1 Mio de copie + 2 Mio transitoires ×2 + relais 2 Mio,
soit 7 Mio. Les pixels CPU préparés sont libérés après l’upload. Aucun nouveau
chiffre de pic RSS/VRAM ni FPS soutenus n’a été mesuré dans cette révision.

## Limites conservées

Les rares compositions GDI non décomposables restent présentées depuis leur
canvas complet harmonisé : elles n’ont pas d’ombres séparées par acteur. Aucune
ne réactive un personnage RGBA. Le masque sable reste conservateur. Les masques
d’ombre couvrent 121 sprites examinés de six familles ; un gris bas non examiné
est préservé et bloque l’ajout d’un doublon plutôt que d’être effacé par index.
