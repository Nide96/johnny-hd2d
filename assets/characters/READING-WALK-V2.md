# Marche de lecture v2 — correction

Planche générée avec imagegen pour corriger la variation de direction et de
proportions de la v1. Les six cellules correspondent encore à JOHNWALK 24..29.
Elles regardent maintenant à gauche ; le miroir correctif de la v1 est retiré.
Les éventuels miroirs de la VM restent appliqués. Les PNG v1 sont conservés.
Les UV/pivots sont mesurés par tools/inspect_reading_sheets.py, sans retouche.
La référence de mouvement est un contact technique des six sprites originaux ;
la référence de style est johnny-reading-props-v1.png.

Le renderer interpole désormais le pivot logique de la tête et la ligne des
pieds, puis place la pose courante. Interpoler les coins des rectangles de tailles
variables déplaçait brusquement le personnage lors d’un changement de pose.
Un changement de miroir coupe l’interpolation. Les coordonnées et le rythme
VM restent intacts. La marche demeure une interprétation dessinée, sans
contrainte squelettique garantissant des pieds physiquement verrouillés.

## Prompt exact

Production game sprite sheet correction. Exactly SIX full-body Johnny walking poses, THREE columns TWO rows, 1536x1024 transparent RGBA canvas with equal 512x512 cells. No ground, no shadow, no halo, labels or scenery.
Image 1 is the six original animation poses in EXACT order JOHNWALK24,25,26,27,28,29. This is the authoritative motion and viewing-angle reference. Match each cell's body direction, chest angle, foot placement and knee bend. All figures face LEFT and TOWARD viewer, three-quarter FRONT view; the chest stays visible in every frame.
Image 2 is the approved modern Johnny identity: warm orange skin, dark brown beard/hair, white sailor cap/navy band, cream ragged shirt, blue ragged shorts, bare feet. Use its clean chunky pixel art and colors, NEVER original red shirt/white shorts.
Critical correction: a SINGLE consistent character rig, SAME head, face direction, cap, shoulder width and torso proportions in every cell. Lock upper-body design/view across all six. Only arms and legs progress smoothly through the walk. Do not change to side or back view at any frame. No twisting torso or alternate outfit. Frame0 legs crossing close, frame1 wide left step, frame2 planted lead foot with rear knee bent and heel lifting, frame3 close passing with opposite arm forward, frame4 other wide step, frame5 other rear heel raised. Precisely follow the corresponding six original reference poses; they form one closed cyclic walk, not six unrelated poses.
Keep consistent cap height, body scale and planted-foot baseline within the six equal cells. Both arms remain anatomically attached; legs alternate naturally, no repeated same support leg on all frames. Fully visible feet and wide transparent gutters. No gradients behind sprites.
