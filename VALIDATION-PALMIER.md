# Correction du palmier — 18 octobre 2026

Trois défauts de présentation corrigés :

- Le moyeu des palmes suivait x455 (pied), au lieu du sommet incliné (445,148). Recalage horizontal et vertical dans le shader.
- La couronne prépeinte contenait aussi du bois : exclusion de cette bande centrale sous les noix de coco, sans retoucher les PNG/JCR.
- TRUNK.BMP et BACKGRND.BMP avaient 157 pixels de masque différents pour le tronc, ainsi que des couronnes 152×75 et 152×69 décalées d’une unité. Le rendu moderne partage maintenant le masque et les dimensions du décor. Les déplacements, les retournements, le découpage et l’ordre des scripts restent conservés.

L’écorce est échantillonnée sur sa largeur complète et adaptée à chaque ligne du masque courbe. Le pied (455,293), les collisions, la VM et le rendu classique ne changent pas.

## Vérifications

- Compilation C99 et régénération des quatre shaders SPIR-V réussies.
- Suite stricte complète réussie : 79 scènes, 30 537 images de capture exactement identiques au canvas VM.
- Test du recalage sur plusieurs origines, déplacement de script, masque canonique, retournement et découpage ; également réussi sous AddressSanitizer/UndefinedBehaviorSanitizer. La suite complète n’a pas été relancée sous sanitizers pour cette modification.
- Capture de notre fenêtre Vulkan : `preview-palm-fixed.png`, scène 34. Inspection du raccord et de la nouvelle écorce.
- Exécutions natives de 240 images : scène 20 (tronc), marche isolée et binaire compact en plein écran. Sortie 0, aucune erreur de validation Vulkan rapportée.
- Scène 52, seed 4242, départ au pas 680 : 240 images soumises, jusqu’au pas VM 751 ; redessin des palmes compatible moderne, sortie 0, aucune erreur de validation rapportée.
- La scène 26 conserve son repli classique prévu ; elle ne sert pas à prouver le rendu moderne des palmes.

Les assets sont des illustrations 2.5D ; cette correction ne transforme pas le palmier en géométrie 3D. Aucun débit soutenu à 144 FPS n’est revendiqué.
