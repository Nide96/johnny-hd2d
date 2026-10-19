# Validation du paquet allégé — 19 octobre 2026

## Extension des masques d’ombres grises

La couverture passe de 142 à 551 poses réparties dans 48 familles : 91 324
pixels d’ombre index 8, contre 24 552 auparavant. Les 142 masques précédents
restent identiques. Le générateur utilise une liste de poses inspectées et
vérifie la géométrie basse, aplatie et externe des nouvelles composantes.
Les journaux, boîtes, rochers, requins, avions et changements de vêtements
repérés lors de l’inspection sont exclus. Les ombres auditées sont supprimées
de la copie GPU même lorsque le découpage ou la position des pieds empêchent
une nouvelle projection ; le canvas classique reste intact.

- make test réussi : 79 scènes, 30 537 images logiques identiques.
- Test hybrid ASan/UBSan réussi ; préservation des 61 et 90 pixels gris du
  haut des deux premières poses de jogging, ainsi que du papier et des rochers.
- Douze captures Vulkan : avant/après sur feu, radeau, bouteille et jogging,
  comparaison classique avant/après, nuit et option --johnny-hd.
  Zéro erreur de validation ; classique identique pixel par pixel.
- Les rares replis composites conservent le canvas complet original : cette
  extension ne prétend pas corriger toutes leurs ombres intégrées.

## Vaguelettes et reflets de la mer

Trois trains de vagues croisés produisent des pentes analytiques pour éclairer
l’eau. La perspective resserre les rides vers l’horizon ; un filtre par dérivées
atténue les fréquences sous le pixel. Les reflets sont fragmentés et la
déformation de l’illustration est réduite. La zone de remplacement du reflet
photographique utilise une portion de mer réfléchie à son échelle horizontale,
au lieu d’étirer une colonne de pixels en rubans. Aucun asset n’est ajouté.

- Compilation C99 stricte et génération SPIR-V réussies.
- make test : 79 scènes, 30 537 images logiques exactement identiques.
- Huit captures Vulkan : jour, coucher du soleil, nuit, animation à deux
  secondes, scintillement nocturne, ancien décor, bouclage à 4096 s et feu.
  Zéro erreur de validation ; bouclage identique pixel par pixel.
- Sable intérieur : variation temporelle au percentile 99 de 1 niveau sur 255
  (éclairage existant des nuages), sans déplacement du sol.
- Radeon 780M, rendu 1920 × 1080 : passe graphique moyenne 1,70 ms le jour,
  1,72 ms la nuit et 1,76 ms avec feu, sur 209 mesures après préchauffage.
  Ces temps excluent le travail CPU, l’upload et la présentation/vsync.
- Aucun code C, script VM, sprite ou traitement audio modifié. ASan/UBSan
  non réexécutés pour cette modification limitée au shader et à sa compilation.

## Correction des ombres de noix de coco

Les composantes d’ombre grises de 21 poses MJCOCO.BMP sont désormais masquées
sur la copie GPU avant la projection de l’ombre moderne. Les gris de la
casquette, des vêtements et les fragments de corps restent intacts.

- make test réussi, dont reconstruction exacte des 79 scènes / 30 537 images.
- Test hybrid ciblé ASan/UBSan réussi : 142 masques, 24 552 pixels identifiés.
- Captures Vulkan avant/après de MJCOCO:1, scène 6, pas 12 ; ombre opaque
  remplacée par l’ombre translucide existante, zéro erreur de validation.
- Comparaison classique avant/après identique pixel par pixel ; contrôle de
  nuit également sans erreur. Aucun changement du canvas, du RNG ou des scripts.
- Aucun nouvel asset PNG/JPEG ; seules les données géométriques du masque
  et les tests/générateur sont étendus.

## Périmètre du nettoyage précédent

Nettoyage des assets inutilisés, médias historiques, caches et outils de
planches retirées. Aucun changement du canvas logique, des scripts, du RNG,
de l’audio ou des shaders actifs. Lors du nettoyage, l’exécutable reconstruit était identique octet
pour octet au binaire précédent (SHA256
`f907125c3f3384dee8e0899d9fac71f646e86346fd587966cad393ebe4754590`).
Le shader character.glsl non référencé est retiré de la cible make shaders.

## Vérifications après retrait

- Compilation C99 stricte : -Wall -Wextra -Wpedantic -Werror.
- make test réussi ; 79 scènes et 30 537 images du canvas logique comparées
  exactement, plus tests de ressources, audio, configuration, capture, lecture,
  pêche, ombres et environnement.
- make shaders réussi pour les trois shaders actifs.
- Paquet minimal : classique et moderne en plein écran, 240 images chacun sur
  Radeon 780M/RADV, couche de validation Vulkan, aucune erreur.
- L’unique décor JCR conservé est identique à celui utilisé avant nettoyage.
- Manifestes SHA256 vérifiés après génération des archives.

Les tests ASan/UBSan avaient réussi au jalon précédent ; lors du nettoyage
ils n’avaient pas été réexécutés pour ce retrait sans changement fonctionnel.
Le test des masques a depuis été réexécuté avec ASan/UBSan pour MJCOCO.
Les mesures précédentes du GPU ne constituent pas une garantie de 144 FPS.
La reconstruction exacte du canvas ne remplace pas une inspection visuelle
exhaustive de chaque scène moderne. Les replis composites et masques
conservateurs du renderer restent inchangés.

## Reproduire

Avec les données originales fournies localement :

```sh
make
make test
make sanitize TESTDIR=work/sanitize
make shaders SHDC=/chemin/vers/sokol-shdc
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./johnny-hd2d --modern --mute --smoke
```

Les sorties de test restent dans work/ et ne sont pas nécessaires à l’exécution.
Les détails du gain de taille sont dans SIZE.md. Les anciens gros fichiers
restent récupérables dans l’historique Git ; aucune réécriture forcée n’a lieu.
