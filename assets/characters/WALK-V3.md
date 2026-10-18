# Marche v3 — contacts et coordonnées

Nouvelle variante générée avec l’outil intégré **imagegen**, conservée dans
`johnny-walk-v3.png`, avec son JCR1 et ses métadonnées JSON. Le fichier source
n’est pas retouché par les outils Python ; ceux-ci convertissent le format et
mesurent l’alpha, les pivots et les semelles. La planche v2 reste disponible.

La caméra est désormais celle de l’original, sans miroir et sans décalage spécial
pour la démo. La taille visible du nouveau personnage suit la hauteur logique
originale projetée, au lieu d’une hauteur arbitraire de 96 unités. Son pivot
horizontal est aligné sur celui de la tête du sprite original (coordonnées
extraites par `tools/export_walk_anchors.c`). Les changements de largeur de pose
ne déplacent donc plus le personnage au moyen d’un simple centre de rectangle.

Chaque pose a un point d’appui calculé sur ses quatre dernières rangées opaques.
L’ombre est placée sous cet appui, avec une largeur liée à la semelle. Le rythme
et les positions restent ceux de la VM. La planche améliore certains bras et
poses de passage ; elle reste un essai artistique, sans garantie de verrouillage
physique parfait du pied à chaque instant. Les autres actions sont originales.

L’éclairage ajouté est une légère teinte du coucher de soleil venant de droite.
Le feu peint en permanence dans le premier décor a été retiré afin de laisser
les accessoires narratifs suivre les scènes de la VM.

## Prompt exact de révision

Use case: precise-object-edit. Asset: revised 8-frame pixel-art walk cycle for a game. Keep the EXACT same Johnny identity, face, white cap, dark brown beard, cream shirt, blue shorts, bare feet, pixel size and outline style. Keep canvas1536x1024 and 4 columns x2 rows. Improve only gait mechanics. Row1 four phases with NEAR leg leading, row2 same four phases with FAR leg leading, not duplicates. Sequence contact, down/recoil, passing, up. Each pose must show opposing arms: NEAR arm swings BACK when NEAR leg leads, FAR arm forward; reverse in second half. Keep a relaxed modest stride, no exaggerated raised knees or marching. At least one foot stays grounded at local y480 in EVERY cell, head top around y85 with small bob max5px. Pelvis stays near local x192 in every frame. Same figure height around395px across ALL 8 frames and same proportions. In contact frames both legs extend naturally, planted foot sole flat; passing frames support leg straight under hips, lifted foot just clear of ground. No foot sliding implied. True transparent background outside isolated characters, opaque sprite interiors. No brown halo, lighting effects, checkerboard, cast shadow, ground, text or grid. Preserve the artist-approved appearance; do not redesign character. This is a production animation sheet, consistency and believable foot contacts take priority.

