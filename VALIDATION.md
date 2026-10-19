# Validation du paquet allégé — 19 octobre 2026

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
