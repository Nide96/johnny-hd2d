# Validation du paquet allégé — 19 octobre 2026

## Périmètre

Nettoyage des assets inutilisés, médias historiques, caches et outils de
planches retirées. Aucun changement du canvas logique, des scripts, du RNG,
de l’audio ou des shaders actifs. L’exécutable reconstruit est identique octet
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

Les tests ASan/UBSan avaient réussi au jalon précédent ; ils ne sont pas
réexécutés pour ce retrait de fichiers sans changement de code fonctionnel.
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
