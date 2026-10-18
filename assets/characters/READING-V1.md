# Lecture v1 — assets et correspondances

Génération avec le skill imagegen et l’outil intégré, le 18 octobre 2026.
Trois PNG RGBA intacts, accompagnés de JCR1 convertis sans retouche :
- johnny-reading-v1 : MJREAD 0..23, 24 cellules (dont corps tronqué 19 et feuille 20).
- johnny-reading-props-v1 : MJREAD 24..35, 12 cellules, dont 5 corps entiers
  (26..30), casquette 24, fragment vertical avec casquette 25, eau 31..34, éclat 35.
- johnny-reading-walk-v1 : JOHNWALK 24..29, six transitions d’entrée/sortie.

Ce lot ajoute donc 33 poses de corps entiers et 9 fragments/accessoires,
soit 42 dessins. Avec le lot précédent : 75 corps entiers et 9 fragments,
84 dessins répartis dans sept planches de personnage/accessoires.

Les références de style sont les poses approuvées de johnny-walk-v3.png.
Les références de mouvement sont les planches de diagnostic MJREAD et JOHNWALK
décodées localement ; ces images originales ne sont pas distribuées dans l’archive.

Le prompt demandait une marche oblique vers la gauche ; la génération a produit
des figures orientées à droite avec une perspective plutôt latérale.
Le renderer les réfléchit, puis applique le miroir demandé par la VM.
Les directions logiques sont conservées ; la perspective est une approximation
artistique qui pourra être affinée. Les gestes dessinés ne reproduisent pas
chaque pixel ou articulation de 1992. L’ajustement des corps au rectangle
source peut légèrement modifier leurs proportions.

tools/inspect_reading_sheets.py mesure l’alpha >=128, trouve des gouttières
transparentes et génère les régions UV et pivots, sans déplacer ni retoucher
les pixels. Des halos de faible alpha peuvent rester dans les PNG ; le shader
emploie un seuil de 0,5. La validation des régions évite de découper une main ou
une goutte parce que les cellules générées ne sont pas parfaitement régulières.

generated/reading_bounds.h provient de tools/export_reading_bounds.c.
Il contient les dimensions et rectangles non transparents des sprites originaux,
sans leurs pixels. La présentation projette ces rectangles avec les miroirs :
la largeur de stockage du BMP ne sert pas de largeur visible à une casquette.
Les fragments et effets ne reçoivent pas d’ombre de contact supplémentaire.
Leur placement et leur ordre viennent des commandes TTM.

La scène 34 est le parcours principal : tous ses corps JOHNWALK et MJREAD ont
une correspondance HD. Les autres ressources (ampoule, noix de coco, animaux,
etc.) restent d’origine. Les poses MJREAD 19,21,22 sont définies et raccordées
mais non rencontrées dans les sept parcours de test à la graine 4242.
Les contacts précis des assemblages de la variante 35 restent à affiner :
la correspondance conserve l’implantation des fragments, sans contrainte
squelettique qui garantirait une jonction anatomique exacte.

## Prompts exacts

### Lecture 0..23

Use case: stylized-concept. Asset: production game animation RGBA sprite sheet.
Generate exactly 24 separated cells, SIX columns and FOUR rows, landscape 1536x1024 canvas, equal 256x256 cells. Transparent background, no letters, no labels, no ground shadows, no halo.
Image1 is approved identity/style: chunky polished pixel art, warm orange skin, dark brown hair and full beard, white sailor cap with navy band, ragged cream shirt, blue ragged shorts, bare feet. Match this character consistently. Image2 is an original MJREAD sprite contact sheet: POSE AND ORIENTATION reference ONLY. Do not copy its red shirt or white shorts.
Recreate MJREAD0..23 in exact row-major order. All seated figures face LEFT in a slight front three-quarter view, feet extending to LEFT, body at right. Use same body proportions and seated leg arrangement across frames. Recreate original gestures and props:
Row1: 0 seated with right arm raised above head holding tilted sailor cap, open grey book in lap; 1 same seated, raised hand lowering cap toward head; 2 hand at cap brim adjusting it; 3 seated, cap on head, one hand near shoulder; 4 seated, hand crossing chest; 5 seated, hand lowering toward open book.
Row2: 6 seated, cap cocked sideways, one hand gesturing, book low in lap; 7 seated with head bent LOW over book, cap tilted forward; 8 seated reading, book in lap, neutral eyes; 9 same with downward glance; 10 same with tiny head tilt; 11 same relaxed reading, eyes on book.
Row3: 12 seated holding a large open grey book up in front of torso, pages wide; 13 book tilted left, right hand turning page; 14 book lifted toward face; 15 book held upright just below eyes, surprised look; 16 book angled the other way; 17 book lowered toward lap, right hand lifted near its upper edge.
Row4: 18 seated closing/lowering book, one hand across chest; 19 ONLY seated torso/arms/blue shorts/legs BELOW NECK, deliberately NO head or cap, original is a separate-body fragment; 20 ONLY a broad diagonal green tropical palm LEAF, same shape and angle as MJREAD20, NOT a character; 21 seated reading with head and cap tilted LEFT; 22 seated, cap cocked and book lifted, face left; 23 seated reading BAREHEADED, brown hair visible, cap completely absent.
Leave generous transparent margins within every cell and never overlap cells. Body poses fit full inside each cell. The leaf and headless body are intentional separate assets, not mistakes. Maintain cream shirt/blue shorts for all character parts. Clean sharp cutout alpha. No scenery, floor, text, extra characters, no red shirts.

### Lecture 24..35 et effets

Use case: stylized-concept. Production game RGBA cutout sprite sheet: exactly TWELVE separate cells, FOUR columns THREE rows, 1536x1024 canvas, equal 384x341.33 cells, generous transparent gutters. Background truly transparent; no labels, letters, backdrop, halo or ground shadows.
Image1 is approved character identity/style: polished chunky pixel art, orange warm skin, dark brown hair and full beard, ragged cream shirt, blue ragged shorts, barefoot, white sailor cap with navy band. Image2 is the original game MJREAD contact sheet, used for pose/orientation and prop silhouettes ONLY. Never copy its red shirt/white shorts.
Recreate original MJREAD24..35 IN THIS EXACT ROW-MAJOR ORDER:
Row1: (24) a single white sailor CAP ONLY with navy band, tilted down left, no head, no person; (25) a single VERTICAL raised warm-skinned FOREARM with HAND supporting a white sailorcap with navy band at the top, NO head, NO torso, NO legs. Match original arm+cap fragment; (26) full Johnny seated facing LEFT with feet extending LEFT, grey open book in lap, cap tipped sideways, match MJREAD26; (27) full Johnny leaning FORWARD facing LEFT, kneeling on right knee, both hands near sand preparing to rise, no book.
Row2: (28) full Johnny on one knee, torso more upright facing LEFT, one hand forward balancing; (29) full Johnny midway getting up facing LEFT, one bent leg planted, one knee raised, arms forward for balance; (30) full Johnny upright standing, front-three-quarter facing LEFT, hands resting near hips, white cap on, cream shirt and blue shorts; (31) ONLY a stylized pale turquoise water splash crown and short central upward jet, no person.
Row3: (32) ONLY a wider water splash with detached droplets spreading left/right; (33) ONLY a flatter lower splash with small scattered droplets and thin water line; (34) ONLY a very wide and shallow elliptical ripple outlined with pale cyan highlights, no filled disk, no background, no character; (35) ONLY a small four-point warm golden twinkle/spark, transparent all around.
The 5 full bodies retain consistent proportions. Match original left-facing poses so the sequence rises from sitting naturally. Props and fragments remain separate, never add a whole character to a cap, arm, splash or sparkle cell. Sharp clean pixel-art alpha, no ground, no text.

### Marche d’entrée et sortie

Use case stylized-concept. Production transparent RGBA pixel-art animation sprite sheet, exactly SIX full-body figures on a 1536x1024 canvas, THREE columns TWO rows of equal 512x512 cells, clean transparent gutters, no writing, no ground, no shadow or halo.
Image1 is approved Johnny identity: white sailor cap/navy band, dark brown beard and hair, warm orange skin, ragged cream short-sleeve shirt, blue ragged shorts, bare feet, clean chunky hand-painted pixel-art edges. Preserve identity and proportions.
Image2 is original JOHNWALK contact sheet, used only as POSE/VIEW reference. Recreate exactly the six frames JOHNWALK24,25,26,27,28,29 in row-major order. All six are the SAME THREE-QUARTER FRONT view walking diagonally TOWARD THE VIEWER AND LEFT. Show the face and chest, never the back. Nose/feet oriented LEFT, nearer shoulder right. Match the alternating leg placements of the original frames:
Top row: 24 left-facing quarter-front walk crossing legs in contact; 25 legs open left step forward, opposite arm forward; 26 other step open, near leg forward left.
Bottom row: 27 passing pose one heel raised; 28 narrow passing pose with opposite leg crossing; 29 recovery pose rear heel raised toward right and forward leg planted left.
These are successive WALKING poses, not standing, running or crouching. Keep same cap height and torso size, every complete body safely inside its own cell, no props. Use cream shirt and blue shorts rather than the original red shirt and white shorts. Transparent alpha.
