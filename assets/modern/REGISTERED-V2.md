# Recalage du décor — version 2

Assets créés avec l’outil intégré **imagegen**, à partir du style approuvé et
surtout des coordonnées de BACKGRND.BMP dans le moteur C99.

- `island-registered-v2.png/.jcr` : sol et mer sans palmier, personnage, radeau
  ou feu permanent. L’illustration reste un fond précalculé ; ses lignes de rive
  sont approximatives. Le shader recale verticalement horizon, arrière et avant
  de l’île à partir de trois correspondances mesurées. Les marées modernes ne
  reprennent pas encore les masques de côte de l’original.
- `palm-registered-v2.png/.jcr` : atlas artistique du palmier. La couronne et
  l’écorce sont échantillonnées séparément par le GPU, sans retoucher le PNG.
- Tronc : **(443,148), 24×145** unités locales, avec le masque original extrait
  dans les commandes de présentation pendant l’exécution. La largeur 22 de la
  zone d’interaction du moteur d’origine n’est pas changée.
- Feuillage : **(366,122), 152×69**, avec sa jonction recalée au sommet incliné **(445,148)**. Le pied reste à (455,293).
- Ombre du palmier : **(397,279), 104×28**, masque original et teinte douce.

Les textures JCR1 contiennent exactement les pixels RGBA8 de leur PNG source.
Le tronc n’est pas placé à l’œil dans une photographie : il partage la caméra
`layout.h` avec les acteurs. Les traces `GFX_SCENERY` ne dessinent rien dans le
canvas de la VM. Les traces `GFX_OCCLUDER` retrouvent leur place dans la liste
moderne pour rétablir l’occultation de la marche, selon la décision du moteur.

Échantillonnage du palmier corrigé : écorce y780..1380, largeur complète adaptée à chaque ligne du masque courbe ; couronne y15..760, avec recalage horizontal et vertical du moyeu. Le morceau de tronc contenu dans la couronne est exclu par le shader. Les redessins TRUNK.BMP partagent désormais les masques BACKGRND.BMP, en conservant déplacement et découpage du script. Une inspection a confirmé un alpha d’au moins
251/255 sur la bande d’écorce. Le seuil 0,5 élimine le halo résiduel autour des
feuilles ; ce nettoyage est une opération de rendu, pas une modification du PNG.

Les anciens fichiers `island-sunset-v1.*` sont conservés comme références et ne
sont plus le décor par défaut. Aucun pixel du jeu original n’a été réécrit.

## Prompt du fond

Use case: compositing. Asset: clean environment background plate for a Johnny Castaway remaster. Image 1 is the STRICT spatial layout guide for a 1280x720 game viewport. Image 2 is ONLY the lighting/material/style reference. Image 3 shows the original game geometry. Create an elegant realistic 3D-looking sunset ocean and EMPTY sandy island matching image 1's composition exactly. Keep the island in the lower middle: open sand footprint from x298 to989, y415 to594, with its back beach around y425, front beach y585, one small isolated rock around x180 y540. Horizon y240. There must be flat open walkable sand around x580..755 y435..470 and to its left and right. Preserve the topology and proportions of the original game. The magenta palm boxes and marker in the guide are technical markers ONLY: do NOT render them. The palm will be added as a separate game layer: NO palm, NO stump, NO plants or rocks near x667 y450. Remove ALL text, symbols, guide lines. Match image 2's refined deep teal organic sea, subtle foam, warm golden light and calm clouded sunset. Keep the elevated camera game view, not an aerial top-down view. NO people, NO raft, NO campfire, NO objects, NO furniture, NO decorative vegetation blocking the island, NO shadows of absent trees, NO UI or watermark. Island surface should remain simple, uncluttered and usable for original animation interactions. Requested output 1280x720 landscape.

## Prompt du palmier

Use case: background-extraction. Asset: isolated palm tree texture for an HD-2D game, genuine transparent alpha background. Use the palm in image 1 only as material/style reference, and image 2 original game palm as silhouette/pose reference. Generate ONE full palm tree, centered, upright almost perfectly vertical thin trunk, no dramatic leaning, no ground or rocks. Canvas1024x1536. Frond crown contained approximately x80..940 y60..540, broad shallow crown with elegant dark green hanging fronds and a few coconuts at center. All the actual foliage must be ABOVE y600; the crown will be cropped and rendered separately by the game. Bare trunk starts around x512 y400 and extends straight down to y1450. The trunk must stay centered at x512 throughout y600..1400, with opaque bark at x490..534, a subtly wider base, realistic warm brown ringed coconut bark, nuanced warm sunlight on the right and cool shadow on left. The game must sample a continuous narrow strip of bark; ensure that central strip is solid opaque wood, never transparent. Full tree incl tips and base fit with at least50px transparent padding. Premium realistic stylized 3D-render appearance matching the coastal sunset reference, restrained detail. No environment, shadow, haze, vignette, glow, text, grid or other plants. Transparent alpha outside the tree.

