# Johnny — essai de marche HD

Planche produite avec l’outil intégré **imagegen**, en prenant comme référence
le personnage du concept `johnny-castaway-3d-lisibilite-v3.png`.
Aucun sprite Sierra n’est remplacé sur disque. L’essai utilise huit nouvelles
poses de profil, avec casquette blanche, barbe brune, chemise crème et short bleu.

`johnny-walk-v2.png` est l’image source générée, conservée sans retouche.
`johnny-walk-v2.jcr` contient exactement ses pixels RGBA8 au format JCR1.
`johnny-walk-v2.json` décrit les limites opaques et pivots de chaque pose ;
`generated/johnny_walk.h` contient les mêmes coordonnées pour le GPU.
Le shader rejette l’alpha inférieur à 0,5 pour éliminer le halo translucide
résiduel de la génération. Les limites sont mesurées au même seuil.

La génération n’a pas respecté parfaitement l’alignement demandé. Le moteur
compense les marges variables avec un pivot sous la casquette et une hauteur
visible de 96 unités dans la scène 1280×720. La base de chaque pose reste au
point d’appui issu du sprite original. Cette compensation ne modifie pas les
pixels de l’image source. L’animation reste un brouillon artistique : contacts,
bras et transition entre les deux demi-cycles pourront être affinés.

La démo alterne les chemins d’origine 25 et 19, au rythme de la marche C99.
Les IDs 1..8 de JOHNWALK sont associés aux nouvelles poses par
`(id + 4) % 8`, dans l’ordre de marche observé dans les tables. Le retournement
horizontal suit celui d’origine, puis celui de la composition moderne.
Le placement d’essai ajoute (140,20) uniquement à la projection pour garder
Johnny à droite du feu ; la VM garde ses coordonnées originales.

Le shader ajoute une légère teinte chaude du côté du feu et froide à l’opposé.
Une ellipse douce fournit une ombre de contact approximative. Ce n’est pas un
éclairage 3D avec normales ni une ombre projetée physiquement.

Reproduire les données sans génération d’image supplémentaire :

```sh
python3 tools/inspect_walk_sheet.py
python3 tools/pack_plate.py assets/characters/johnny-walk-v2.png assets/characters/johnny-walk-v2.jcr
```

## Prompt de génération

Use case: stylized-concept. Asset type: production transparent 2D pixel-art WALK CYCLE SPRITE SHEET for the Johnny Castaway remaster. Input image is a character identity/style reference only: reproduce the bearded pixel-art fisherman standing on the sand, NOT the island. Create exactly EIGHT full-body poses of this SAME Johnny walking toward the RIGHT in a seamless side-view walk cycle. Layout: exactly FOUR equal columns by TWO equal rows, read left-to-right then top-to-bottom. Canvas 1536x1024; each cell 384x512. Character centered at local x192 in each cell, sole baseline local y480, cap top around y64. At least 32px fully transparent padding all sides. All eight sprites same scale, same face, head size, beard, clothes and proportions. Distinct phases: 1 left leg forward contact, 2 left leg takes weight down, 3 right leg passes supporting left leg, 4 right knee leading up, 5 right leg forward contact, 6 right leg takes weight down, 7 left leg passes supporting right leg, 8 left knee leading up. Alternate arms naturally opposite legs. Side silhouette readable, modest human stride, relaxed stranded sailor gait. Identity: middle-aged castaway with large dark brown beard and moustache, visible friendly nose, dark hair, worn white/cream sailor cap, ragged cream short-sleeved shirt, blue knee-length shorts, barefoot. Cartoon proportions as in reference, big expressive head about one third of height. Premium clean HD pixel art, deliberate crisp pixel clusters and warm limited palette, consistent dark brown outline, readable at 80-100px tall. Neutral diffuse shading, no dramatic baked directional lighting. No fishing rod, tools or held objects. No ground, cast shadow, backdrop, checkerboard, labels, text, numbers, grid lines, extra characters or extra poses. Real transparent alpha background; every sprite isolated with ample transparent gaps. Do not reproduce the reference environment.

## Prompt de correction

Use case: precise-object-edit. Asset: transparent game sprite sheet. Edit this sheet, preserve the EXACT same Johnny character design, clothes, face, colors, crisp pixel art and its 8 walking poses. Fix production cleanup ONLY: remove all brown atmospheric glow, colored haze, shadows, soft halo and every background pixel outside the character silhouettes. Actual alpha zero everywhere outside sprites; solid opaque character interiors, crisp cutout silhouette, no black or colored matte. Keep exact 1536x1024 canvas, exact 4 columns x 2 rows grid of 384x512 cells. Crucial alignment: uniformly position/scale each complete character to SAME visible height about400px, centered x192 within its own cell; standing sole baseline y480 within each cell, cap top about80. Keep small natural knee/body bob only, less8px. Full feet/cap fit in cell with transparent margins. Do not draw grid, labels, ground or any shadow. Preserve distinct walking poses and consistent face and head size. This PNG is intended to be composited into a game with transparency, so no atmosphere behind or around the figure.

