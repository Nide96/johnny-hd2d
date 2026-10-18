# Poses et repos v1 — provenance

Trois planches générées avec l’outil intégré imagegen le 18 octobre 2026.
Aucune retouche artistique après génération. `pack_plate.py` convertit seulement
le format ; `inspect_action_sheets.py` mesure les régions alpha et génère les UV.
Les PNG restent intacts. Le shader applique un seuil alpha de 0,5 pour éliminer
les halos translucides. Les gouttières sont mesurées plutôt que supposées exactes.

Références : `johnny-walk-v3.png` pour l’identité et les planches de diagnostic
JOHNWALK, STNDLAY et SLEEP décodées localement depuis les ressources originales.
Ces planches de diagnostic ne sont pas distribuées dans le projet portable.

Correspondances exactes dans `character_assets.h` :
- JOHNWALK : 0, 9..12, 14..18 (10 poses nouvelles), en plus des 8 marches v3.
- STNDLAY : 0..7 (8 corps entiers).
- SLEEP : 0..11 et 13..16 (16 corps entiers).

La pose générée correspondant à JOHNWALK 11 regarde initialement vers la droite :
le rendu la réfléchit pour retrouver la direction originale gauche, puis applique
l’éventuel miroir demandé par le script. Les poses debout utilisent le pivot de
la tête original et la hauteur originale projetée. Les poses de repos/sommeil sont
ajustées au rectangle logique original, ce qui peut déformer légèrement leurs
proportions. Le clipping, les miroirs et l’ordre des dessins viennent de la VM.
Le petit dessin de main STNDLAY 8, la tête isolée SLEEP 12, les autres directions
de marche et les actions de pêche/lecture restent originaux. Ce lot est une
étape artistique, pas la couverture complète de toutes les animations.

## Prompts exacts

### Poses

Create a production transparent RGBA sprite sheet, FIVE columns and TWO rows, exactly TEN separated full-body character poses. Use image 1 as the approved character identity and rendering style: clean chunky hand-painted pixel-art edges, warm orange skin, dark brown beard and hair, white sailor cap with navy band, ragged cream shirt, blue ragged shorts, bare feet. Preserve this identity in every pose. Image 2 contains original game poses as a motion reference ONLY: never copy its red shirt or white shorts. Output no words, no labels, no floor, no scenery, no shadows, no halo. Keep wide transparent gutters and equal 307.2x512 cells on a 1536x1024 canvas. Center one person per cell, full body entirely visible, head height consistent. Exact order from left to right:
Top row: original JOHNWALK0 standing right profile with arms relaxed; original JOHNWALK9 walking quarter front facing viewer and right; original JOHNWALK10 walking directly toward viewer; original JOHNWALK11 three-quarter rear facing left; original JOHNWALK12 standing full rear.
Bottom row: original JOHNWALK14 full rear walking step; original JOHNWALK15 full rear standing hands on hips; original JOHNWALK16 full front standing hands on hips; original JOHNWALK17 quarter front facing right standing hands on hips; original JOHNWALK18 quarter rear facing left standing with arms at sides.
Faithfully retain these distinct viewing directions; rear views must show back of cap and hair and NO face. Pixel-art animation asset, consistent proportions with image1, feet not cropped, flat transparent background.

### Assise et allongement

Create a transparent RGBA production character animation sheet, exactly FOUR columns TWO rows, EIGHT isolated sprites, no writing. Image1 is the approved character: brown beard and hair, white sailorcap with navy band, ragged cream shirt, blue shorts, barefoot, clean charming chunky pixel-art rendering. Image2 supplies poses ONLY, do not copy its red shirt or white shorts. Match poses STNDLAY0 to STNDLAY7 in order, OMIT STNDLAY8 and9:
row1: (0) standing quarter front facing right with knees beginning to bend; (1) crouching down facing right, arm extended for balance; (2) sitting low on ground facing right leaning back on left hand; (3) falling backward to recline, head at LEFT, both legs lifted toward RIGHT.
row2: (4) lying almost flat face up head LEFT feet RIGHT, knees slightly bent with feet raised; (5) lying flat on back head LEFT feet RIGHT with legs fully stretched, elbows by body; (6) reclined head LEFT feet RIGHT, leaning on elbows, knees gently raised; (7) sitting reclined on left arm head LEFT upper body lifted, legs extending RIGHT.
All eight sprites full body, including cap, hands, feet. Consistent body scale; lying figures become wide and short, NEVER standing in row2. One sprite centered in each equal 384x512 cell on a 1536x1024 canvas with wide transparent gutters. Fully transparent empty space. No floor, ground shadow, backdrop, gradient, glow or props. These are successive animation frames for the original sit/lie/get-up script.

### Sommeil

Create a production transparent RGBA sprite sheet, FOUR columns FOUR rows, SIXTEEN isolated animation poses. Output 1536x1024 landscape canvas with equal 384x256 cells, one complete body centered inside each, broad transparent gutters. No labels, no backdrop, no floor, no ground shadows or halo. Reference1 is character identity/style: warm skin, dark brown beard/hair, cream ragged shirt, blue ragged shorts, bare feet, white sailorcap with dark navy band. Reference2 is original SLEEP.BMP pose contact sheet, for motion ONLY. Never use original red shirt or white shorts. Recreate exact poses SLEEP0..11 then13..16, skip head-only sprite12. Keep head on LEFT and feet RIGHT in EVERY pose; never flip orientation. Use clean polished chunky pixel-art edges and identical body proportions as reference1.
Row1: SLEEP0 seated leaning back on hands legs stretched right; SLEEP1 lying on back head left cap behind head feet right, right arm extended on ground; SLEEP2 lying on back head tilted upward arms close to sides one knee slightly bent; SLEEP3 full rear-side lying pose showing back of head and shirt.
Row2: SLEEP4 lying on side head left face away, straight legs right; SLEEP5 rolling to prone, back visible one leg bent; SLEEP6 on back face partly toward viewer legs apart one hand extended down; SLEEP7 rolling onto side facing viewer one knee bent arm near chest.
Row3: SLEEP8 lying prone face left toward viewer forearm extended downward, right knee bent; SLEEP9 lying on side curled, head left tilted to viewer arm tucked in; SLEEP10 on back like pose1 with closed eyes; SLEEP11 on back like pose1 with half-open eyes.
Row4: SLEEP13 sleeping on back nose raised, closed eyes; SLEEP14 on back chin down looking toward viewer eyes open; SLEEP15 on back mouth open yawning; SLEEP16 upper body lifting from back elbows grounded, feet right legs extended.
NO upright standing figures. Wide horizontally laid-down character except first and final reclined pose. Preserve distinctive beard, cap, cream shirt and blue shorts consistently. Exact 16 cells. Transparent background.
