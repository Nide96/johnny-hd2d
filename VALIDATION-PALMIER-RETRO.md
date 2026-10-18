# Palmier original et gag de lecture — 18 octobre 2026

## Diagnostic confirmé par la source et la capture

Scène 35, graine 1, horloge 2026-10-18T12:00, pas VM 151 :
- Johnny : `MJREAD.BMP:6`.
- Noix de coco : `COCONUTS.BMP:0`.
- Palme du gag : `MJREAD.BMP:20`, 48×52 pixels, position (189,103).

Ce morceau de palme est dessiné dans une pose du gag ; il n’est pas un simple
patch de restauration du fond. L’hypothèse de restauration a été écartée après
décodage et inspection du flux exact. L’illustration du palmier moderne ne
partage pas sa silhouette : le morceau vert devient très visible sur la couronne
peinte. Une pose comique originale ne peut pas constituer à elle seule une
animation cohérente de la maquette photographique.

## Variante livrée

`--palm-original` choisit dès l’initialisation un palmier entièrement R8 pour
**toute l’exécution**, sur le décor HD. Le tronc, la couronne, la palme, les
noix de coco et Johnny partagent alors le vocabulaire visuel d’origine.
Le pipeline applique la LUT moderne et l’éclairage jour/nuit existants.
Les coordonnées, retournements, dimensions et clips viennent des commandes
originales, sans remapper la maquette fixe sur les poses. L’ombre de fond
originale est rendue via le matériau de contact doux au lieu d’un gris opaque.
Les déformations comiques de l’animation originale sont conservées.

Le mode moderne sans l’option garde son palmier peint ; la variante est explicite
pour permettre un choix artistique. Le lanceur `preview-reading-palm.sh`
emploie l’option, scène 35, saut de 125 pas, pour voir rapidement le gag.
Aucune nouvelle planche PNG/JCR artistique. Les PNG ajoutés sont des captures
natives du moteur, pas des ressources de remplacement.

## Vérifications

- `make sanitize` réussi (ASan/UBSan, `detect_leaks=0`).
- Build C99 strict `-Wall -Wextra -Wpedantic -Werror`.
- `make test` réussi, 79 scènes / 30 537 images reconstruites exactement.
- Régression lecture : la vraie scène 35 présente la pose MJREAD20 48×52 ;
  les sept relectures conservent leur canvas classique exact.
- `vm_core.c/.h`, `engine.c`, `gfx.c`, `island.c`, `ttm.c`, `resource.c`,
  `render_frame.c/.h` et le recalage palmier sont identiques à la livraison
  précédente. Le changement se limite à la présentation et aux options.
- Radeon 780M / RADV PHOENIX : 4 essais de 240 images, lecture/chute,
  suite de lecture, couronne TTM scène 52 et classique. Aucune erreur Vulkan.
- Deux captures figées au même pas et éclairage, fenêtre propre uniquement :
  `preview-palm-original-comparison.png`, détail `preview-palm-original-detail.png`.
- Le mode moderne par défaut est pixel-identique à sa capture précédente au
  même état VM : la variante n’altère pas le rendu de comparaison.

Le diagnostic artistique est établi sur ce gag. Les autres scènes ont une
couverture de canvas complète ; leur aspect final n’est pas revendiqué comme
visuellement inspecté image par image. Journaux `validation/palm-retro-*`.

ELF actuel : 239 416 octets ; version compacte : 89 856 octets.
