# Johnny rétro — torse nu, short blanc et casquette

Direction demandée par l’utilisateur : retrouver le côté rétro et comique du
personnage avec seulement un short blanc et une casquette, en conservant la barbe.

Sept planches réinterprétées avec l’outil intégré imagegen. La marche de profil
sert de référence commune : torse nu, short blanc simple, casquette blanche,
barbe et moustache foncées, cheveux en bataille, nez rond, petit ventre et grands
pieds. L’expression reste volontairement perplexe/pataude. C’est une interprétation
inspirée du personnage, pas une restauration pixel à pixel ni une palette EGA
strictement limitée à seize couleurs.

Les 75 corps entiers et les neuf fragments/accessoires déjà raccordés sont
conservés. Les indices, cellules, miroirs logiques, scripts, cadence et caméra
ne changent pas. Les corps partiels restent partiels. Les anciens PNG/JCR sont
conservés comme historique et ne sont plus chargés dans ces sept emplacements.
Les familles d’origine encore sans remplaçant restent d’origine.

`active-retro.json` décrit les sept planches actives. `inspect_retro_sheets.py`
mesure l’alpha >=128, vérifie les gouttières et les limites, génère les trois
headers et les JSON, puis convertit exactement les pixels RGBA en JCR1.
Les PNG générés ne sont pas retouchés. Des halos de faible alpha peuvent exister
hors des silhouettes ; le shader conserve son seuil de 0,5.
Les trois anciens outils de mesure délèguent à cet outil lorsque le manifeste
rétro est présent, afin de ne pas régénérer des UV pour des textures inactives.

Les correspondances et le réglage d’interpolation tête/ligne de pieds sont ceux
des jalons précédents. Aucun nouveau contrôle squelettique ou physique n’a été
ajouté. Les contacts de fragments restent soumis aux mêmes limites artistiques.

## Prompts exacts (outil intégré)

### Marche de profil, référence commune

Use case: identity-preserve / game sprite sheet redesign.
EDIT IMAGE 1, maintaining its EXACT EIGHT walking poses, positions, facing RIGHT, FOUR columns TWO rows, equal cells, 1536x1024 RGBA canvas and transparent gutters. Every body part, foot phase and cap direction must stay in its corresponding cell. Image2 is the original 1992 Johnny silhouette/personality reference, not a color template.
Reinterpret the character to faithfully restore Johnny Castaway's funny retro castaway appearance: ONLY plain WHITE boxer-style SHORTS, a slightly wonky WHITE SAILOR CAP with dark narrow band, and BARE FEET. Completely BARE CHEST and ARMS, no shirt, no vest, no shoes, no belt, no dangling rope. Slightly round soft belly, lanky thin limbs, oversized bare feet, relaxed slouch, a little clumsy and endearing. Same very large DARK BROWN/NEAR BLACK SCRUFFY BEARD, moustache, dark messy hair and big bulbous reddish nose. Expressive slightly puzzled eyes and eyebrows with comic deadpan charm. Adult man, not a baby, not muscular, not heroic.
Art: crisp chunky nostalgic pixel art, restrained 12–16-color look, black/dark outline, warm tan/sunburned skin, two or three flat tone clusters per material, white shorts with subtle grey pixel shading. Preserve recognizable cap/beard/head scale of original, beard hanging over upper chest. A tiny potbelly peeks below beard. Bare torso must be unmistakable on EVERY sprite. White shorts at upper thighs, absolutely NO BLUE clothing and NO CREAM SHIRT anywhere.
Keep existing motion and cell order, consistent head/body proportions. Transparent alpha background with no glow, no scenery, no floor, no ground shadow, no letters or labels. Clean usable production sprite sheet.

### poses

EDIT IMAGE1 as a production game sprite sheet. Image1 is the edit TARGET: preserve EXACT cell count, cell order, poses, gestures, view directions, positions, transparent gutters and 1536x1024 RGBA canvas. Image2 is the APPROVED NEW CHARACTER identity reference ONLY, ignore its walking poses. Apply its bare-chested retro Johnny design consistently: only plain WHITE boxer-style shorts, WHITE sailor cap with dark narrow band and bare feet. NO shirt, NO vest, NO blue shorts, NO belt or rope. SAME large dark scruffy beard/moustache, tousled hair, bulbous reddish nose, slightly round belly, thin limbs, big bare feet and funny puzzled/deadpan expression. Adult castaway with original 1992 charm, crisp chunky pixel art with limited flat tone clusters and dark outlines, not muscular or photorealistic. Restore bare skin wherever the old cream shirt was. Preserve cap handling and any intentionally bareheaded figure. Keep every animation pose and all accessories. Actual transparent background, no glow, floor, labels or added shadows. TEN poses, FIVE columns TWO rows. Preserve every front, side, quarter-back and full-back orientation, hands-on-hips gestures, and each leg phase. Back views MUST remain back views with NO face and bare back above white shorts.

### rest

EDIT IMAGE1 as a production game sprite sheet. Image1 is the edit TARGET: preserve EXACT cell count, cell order, poses, gestures, view directions, positions, transparent gutters and 1536x1024 RGBA canvas. Image2 is the APPROVED NEW CHARACTER identity reference ONLY, ignore its walking poses. Apply its bare-chested retro Johnny design consistently: only plain WHITE boxer-style shorts, WHITE sailor cap with dark narrow band and bare feet. NO shirt, NO vest, NO blue shorts, NO belt or rope. SAME large dark scruffy beard/moustache, tousled hair, bulbous reddish nose, slightly round belly, thin limbs, big bare feet and funny puzzled/deadpan expression. Adult castaway with original 1992 charm, crisp chunky pixel art with limited flat tone clusters and dark outlines, not muscular or photorealistic. Restore bare skin wherever the old cream shirt was. Preserve cap handling and any intentionally bareheaded figure. Keep every animation pose and all accessories. Actual transparent background, no glow, floor, labels or added shadows. EIGHT poses, FOUR columns TWO rows: bending, crouching, sitting, falling back, lying on back and leaning up. Preserve those poses exactly, head LEFT feet RIGHT for reclined figures. Never change lying figures into standing figures.

### sleep

EDIT IMAGE1 as a production game sprite sheet. Image1 is the edit TARGET: preserve EXACT cell count, cell order, poses, gestures, view directions, positions, transparent gutters and 1536x1024 RGBA canvas. Image2 is the APPROVED NEW CHARACTER identity reference ONLY, ignore its walking poses. Apply its bare-chested retro Johnny design consistently: only plain WHITE boxer-style shorts, WHITE sailor cap with dark narrow band and bare feet. NO shirt, NO vest, NO blue shorts, NO belt or rope. SAME large dark scruffy beard/moustache, tousled hair, bulbous reddish nose, slightly round belly, thin limbs, big bare feet and funny puzzled/deadpan expression. Adult castaway with original 1992 charm, crisp chunky pixel art with limited flat tone clusters and dark outlines, not muscular or photorealistic. Restore bare skin wherever the old cream shirt was. Preserve cap handling and any intentionally bareheaded figure. Keep every animation pose and all accessories. Actual transparent background, no glow, floor, labels or added shadows. SIXTEEN poses, FOUR columns FOUR rows, mostly horizontal sleeping bodies. Preserve all sleeping positions, eyes, mouth, head/feet directions and reclining gestures, including rear views: their backs are BARE skin, not shirts.

### reading

EDIT IMAGE1 as a production game sprite sheet. Image1 is the edit TARGET: preserve EXACT cell count, cell order, poses, gestures, view directions, positions, transparent gutters and 1536x1024 RGBA canvas. Image2 is the APPROVED NEW CHARACTER identity reference ONLY, ignore its walking poses. Apply its bare-chested retro Johnny design consistently: only plain WHITE boxer-style shorts, WHITE sailor cap with dark narrow band and bare feet. NO shirt, NO vest, NO blue shorts, NO belt or rope. SAME large dark scruffy beard/moustache, tousled hair, bulbous reddish nose, slightly round belly, thin limbs, big bare feet and funny puzzled/deadpan expression. Adult castaway with original 1992 charm, crisp chunky pixel art with limited flat tone clusters and dark outlines, not muscular or photorealistic. Restore bare skin wherever the old cream shirt was. Preserve cap handling and intentionally bareheaded figures. Actual transparent background, no glow, floor, labels or added shadows. TWENTY-FOUR cells, SIX columns FOUR rows. Preserve the grey book and all exact reading/hand/cap gestures. Cell19 (row4 col2) deliberately has NO HEAD OR CAP: keep this separate seated torso/legs fragment HEADLESS with bare torso and white shorts. Cell20 (row4 col3) is ONLY a green LEAF: keep it as a leaf, do not add a character. Cell23 (row4 col6) is BAREHEADED: retain hair and no cap. Keep all original viewing directions and leg placement.

### reading-props

EDIT IMAGE1 as a production game sprite sheet. Image1 is the edit TARGET: preserve EXACT cell count, cell order, poses, gestures, view directions, positions, transparent gutters and 1536x1024 RGBA canvas. Image2 is the APPROVED NEW CHARACTER identity reference ONLY, ignore its walking poses. Apply its bare-chested retro Johnny design consistently: only plain WHITE boxer-style shorts, WHITE sailor cap with dark narrow band and bare feet. NO shirt, NO vest, NO blue shorts, NO belt or rope. SAME large dark scruffy beard/moustache, tousled hair, bulbous reddish nose, slightly round belly, thin limbs, big bare feet and funny puzzled/deadpan expression. Adult castaway with original 1992 charm, crisp chunky pixel art with limited flat tone clusters and dark outlines, not muscular or photorealistic. Restore bare skin wherever the old cream shirt was. Preserve cap handling and intentionally bareheaded figures. Actual transparent background, no glow, floor, labels or added shadows. TWELVE cells, FOUR columns THREE rows. The ONLY whole bodies are cells2,3,4,5,6 (reading, kneeling, rising, standing); restyle those bare-chested with white shorts. Cell0 is ONLY a white cap, cell1 ONLY a forearm/hand with cap: preserve separate fragments with NO added body/head. Cells7,8,9,10 are ONLY water splashes/ripple, cell11 ONLY a gold sparkle: preserve these effects as effects with NO added person. Keep these exact cells and prop layouts.

### reading-walk

EDIT IMAGE1 as a production game sprite sheet. Image1 is the edit TARGET: preserve EXACT cell count, cell order, poses, gestures, view directions, positions, transparent gutters and 1536x1024 RGBA canvas. Image2 is the APPROVED NEW CHARACTER identity reference ONLY, ignore its walking poses. Apply its bare-chested retro Johnny design consistently: only plain WHITE boxer-style shorts, WHITE sailor cap with dark narrow band and bare feet. NO shirt, NO vest, NO blue shorts, NO belt or rope. SAME large dark scruffy beard/moustache, tousled hair, bulbous reddish nose, slightly round belly, thin limbs, big bare feet and funny puzzled/deadpan expression. Adult castaway with original 1992 charm, crisp chunky pixel art with limited flat tone clusters and dark outlines, not muscular or photorealistic. Restore bare skin wherever the old cream shirt was. Preserve cap handling and intentionally bareheaded figures. Actual transparent background, no glow, floor, labels or added shadows. SIX poses, THREE columns TWO rows. All face LEFT and toward viewer, front three-quarter, in their existing six gait phases. Keep this exact orientation in EVERY cell, SAME head/cap/torso proportions across the cycle, preserve foot phases and alternating arms. Do not reverse direction or revert to right-facing walking from identity reference.
