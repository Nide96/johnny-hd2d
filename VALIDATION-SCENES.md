# Validation des scènes et poses v1 — 18 octobre 2026

> Historique du lot repos. La couverture, les tailles et les vérifications actuelles
> sont dans `VALIDATION-LECTURE.md`.


## Changement

34 poses nouvelles (10 orientations/attitudes, 8 assises/allongements,
16 poses de sommeil), soit 42 poses HD avec les 8 marches existantes.
Le choix dépend désormais du nom exact de la ressource BMP et de son index,
y compris dans les scripts TTM. Les noms sont normalisés lors du chargement,
restaurés après chaque dessin et copiés dans les instantanés possédés.
Décharger ou réutiliser un slot TTM ne change plus l’identité d’un dessin capturé.
Un changement de famille coupe l’interpolation de position entre deux dessins.

Les dessins TRUNK.BMP 0/1 emploient aussi le palmier moderne à l’emplacement
et au rang de superposition demandés par les scripts, avec clipping et miroirs.
Aucun opcode, tirage aléatoire, délai ni coordonnée de la VM n’a été modifié.

`preview-scenes-hd.sh` démarre la scène originale 51. On peut passer 52 ou 53,
ou tout numéro 1..79 ; l’histoire continue après la scène forcée.
`--skip-steps N` avance de N pas VM avant la présentation et exige `--scene`.
Ces essais ne sauvegardent pas l’INI. Ils ne sont pas la démo de marche isolée.

## Vérifications exécutées

- Suite C99 stricte avec ASan/UBSan : ressources, VM, ordonnanceur, INI,
  capture, marche et caméra réussis. Reconstruction exacte du canvas C pour
  **79 scènes / 30 537 images**, sans saturation d’atlas.
- Nouveau test `test_character.c`, également ASan/UBSan : 42 correspondances,
  exclusions des fragments, réutilisation/déchargement d’un slot, identité
  possédée après destruction des ressources et des contextes ; scène 51 réelle
  traversant les quatre planches, canvas classique toujours exact.
- Les trois nouvelles planches ont été inspectées. Leur outil de métadonnées
  vérifie qu’aucun pixel d’alpha >=128 n’est coupé par les régions de découpe.
  Le PNG lui-même peut conserver un halo d’alpha faible ; le shader l’écarte.
- Deux binaires : scénario de 1 000 pas, graine 1, midi, hash inchangé
  `de8bfcfeb48c1c6b`, zéro repli pour saturation d’atlas.
- Avance de scène : 80 pas puis 1 pas vérifié donnent bien 81 pas VM,
  278 ticks, zéro scène terminée dans l’essai choisi.
- Radeon 780M, Vulkan avec couche de validation : sept essais de 240 images
  présentées, sorties 0, aucune erreur de validation. Cas : assise, sommeil,
  scène de pêche redessinant le palmier, marche, classique, nuit et binaire UPX.
  Le build de diagnostic SOKOL_DEBUG a été utilisé pour les six premiers.
  Seul avertissement observé : DPI X11 non disponible (repli 96 DPI).
- Capture de notre fenêtre après stabilisation : 64 images, huit secondes,
  échantillonnage 8 images/s, puis encodage GIF/MP4. Le journal confirme les
  soumissions des poses debout, assises et de sommeil dans le même scénario.
  Cette cadence de capture ne mesure pas les FPS du renderer.

## Couverture et limites

`assets/scenes/inventory.json` provient de l’exécution des 79 scripts forcés,
graine 4242, horloge 1993-06-01T10:00, chapitre 1. `COVERAGE.md` indique que
51 parcours sur 79 rencontrent au moins une pose remplaçable. **Ce ne sont pas
51 scènes entièrement modernisées.** Les branches peuvent varier avec la graine.

Les corps fragmentés de pêche/lecture, les directions obliques de marche
restantes, la main STNDLAY 8 et la tête SLEEP 12 gardent leurs dessins originaux.
La taille des poses de repos est ajustée au rectangle VM ; ce choix peut étirer
légèrement le dessin. La planche est une interprétation artistique et ne prouve
pas un contact physique exact pour chaque main, pied ou accessoire. Seules les
scènes capturées ont fait l’objet de la revue visuelle actuelle ; le test exhaustif
compare la présentation classique au canvas C, pas au programme Win16.

La caméra commune du jalon précédent est conservée. Nuit et scènes incompatibles
reviennent au classique. La rive exacte, le radeau moderne, le bloom HDR,
l’éclairage des feux et l’audio de périphérique restent à réaliser.

ELF : **165 496 octets** ; UPX : **72 084 octets**, hors données et bibliothèques.
Staging Vulkan porté de 32 à **64 Mio** pour les six textures RGBA ; ce chiffre
n’est pas la consommation mémoire totale. Aucun 144 FPS soutenu n’a été mesuré.
