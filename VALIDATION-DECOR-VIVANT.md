# Décor vivant — validation du 18 octobre 2026

## Rendu livré

Le mode moderne conserve le décor enregistré et les sprites originaux, puis
ajoute une animation GPU continue : houle et rides à plusieurs fréquences,
écume qui avance/reflue au bord du sable, sable humide, ombre légère de nuages,
vent dans les palmes, ciel jour/crépuscule/nuit avec deux couches de nuages,
soleil, lune et étoiles fixes à scintillement lent. Les nuages atténuent les
étoiles et la lune ; les étoiles s’effacent à l’aube et restent dans le ciel.

Le ciel peint de la plaque est remplacé par un ciel procédural. Le reflet
solaire figé de la photo est atténué avant l’ajout du reflet soleil/lune ; le
rocher et le sable ne subissent pas la déformation UV de l’eau.
Le contour du rivage est calculé à partir de la matière sable du décor, par
ouverture, composante connexe, fermeture des trous et distance signée.
Le même masque borne l’écume et les ombres. Son exclusion du rocher est
conservatrice. Les contours sont adaptés à la plaque enregistrée fournie ;
une autre composition de décor demande un nouveau calage.

Les sources FIRE1/FIRE2 et autres feuilles de feu connues sont inspectées
pour des pixels jaunes/rouges visibles. Une vraie flamme active une lueur chaude
sur le sable, les sprites et le palmier, avec un scintillement commun et trois
braises derrière les acteurs. La feuille dédiée FIRE1 garde ses pixels de flamme
émissifs la nuit. La peau, les vêtements blancs et la ponctuation ne déclenchent
pas de halo. Les halos sont calculés directement par émetteur dans les passes
existantes ; ce jalon n’ajoute pas de post-traitement HDR à flou multipasse.

Les attaches des palmes et le tronc restent fixes. Les scènes de contact
(tronc, noix de coco/palme, couronne TTM) verrouillent le vent dès leur début.
L’identifiant du catalogue est transmis comme métadonnée de présentation,
sans modifier la décision du scheduler. La liste conservative provient de
l’inventaire des scènes et inclut les scènes 5–8, 35, 51–53, 76 et 78.
Les mouvements comiques des scripts sont conservés.

## Horloges et modes

L’horloge visuelle vient du temps monotone de la fenêtre, indépendamment du
rattrapage 55/16 ms de la VM. Les phases bouclent à 4 096 s sans saut visible.
`--time-lapse N` accélère uniquement la journée visuelle ; le calendrier du jeu
reste inchangé. `--freeze-scene` garde la VM fixe avec un environnement animé ;
`--decor-time N` fixe le temps graphique pour les tests. Les modes de capture
et de cycle accéléré préservent le fichier INI.

`preview-living.sh` présente une journée en 120 s et choisit le palmier original.
`preview-stars.sh` commence de nuit. Ces deux lanceurs jouent les scènes normales,
sans forcer une boucle de marche. `--no-decor-fx` conserve la comparaison avec
la présentation antérieure. Les replis de composition des scènes rares restent
ceux du moteur précédent ; ils n’activent pas les nouveaux effets. Le classique
conserve sa présentation indexée.

## Vérifications

- Build C99 strict et compilation SPIR-V via le sokol-shdc épinglé.
- `make test` et `make sanitize` réussis, ASan/UBSan avec `detect_leaks=0`.
- Contrôles ciblés supplémentaires sur le modèle atmosphérique, la topologie
  et la distance du masque, le verrou de vent par scène, les vrais pixels de
  flamme et les coordonnées de clip/retournement.
- Les 79 scènes reconstruisent 30 537 images exactement. Le nouvel identifiant
  de scène est vérifié pendant les relectures. Flux 1 000 pas inchangé :
  `de8bfcfeb48c1c6b`, y compris avec les options de rendu.
- Logique de l’île, décodeur, scripts TTM, raster CPU et capture de commandes
  inchangés. `engine.c` et `vm_core.c` ajoutent seulement le transport du numéro
  de scène vers le renderer. Aucun tirage RNG de la VM n’est consommé pour les FX.
- 11 essais natifs de 240 images sur Radeon 780M / RADV PHOENIX : classique,
  jour/nuit, palmier peint, gag de noix de coco, couronne TTM, feu, composition
  spéciale, cycle accéléré, VM figée et comparaison sans FX. Zéro erreur Vulkan.
- Lanceurs livrés (scènes normales, jour/nuit accéléré et nuit immédiate) et
  compact en plein écran : 240 images chacun, zéro erreur Vulkan.
- VM figée : 101 pas conservés pendant 240 images de décor vivant.
- Exécution native feu + masque + audio sous ASan/UBSan : 240 images sans erreur.
- Test GPU de la matière Johnny : peau, blanc, encre, œil, damiers et
  transparence gardent leurs valeurs de référence en mode de comparaison.
- Test GPU du décor : 67 groupes d’étoiles détectés dans le haut du ciel ;
  54 restent repérés au même pixel 32 s plus tard, les autres étant atténués
  par les nuages/seuil. Eau et ciel changent entre t=0 et t=2 ; delta au 99e
  percentile dans le sable intérieur : 1/255. t=0 et t=4096 sont pixel-identiques.
- 245 pixels de flamme émissive détectés dans la capture de nuit du feu.
- Captures limitées à la fenêtre du projet. Vidéo muette de 24 s, 1 280×720,
  60 images/s, enregistrée par son identifiant X11 unique, sans bureau ni micro.

## Coût mesuré sur la 780M

Timestamps Vulkan autour de la passe graphique, plein écran stabilisé avec
viewport **1 920×1 080**, 209 échantillons retenus par cas après échauffement.
Le relevé de résolution est effectué après les événements de redimensionnement
initial. Les valeurs ne comprennent pas le CPU, les uploads d’atlas ou l’attente
VSync ; elles ne constituent pas une mesure de 144 FPS soutenus.

| Cas | GPU moyen (ms) | GPU p95 (ms) |
| --- | ---: | ---: |
| day | 1.636 | 1.684 |
| night | 1.586 | 1.690 |
| reference | 0.480 | 0.551 |
| fire-hd | 1.772 | 1.867 |

Méthode : [timestamps Vulkan, documentation Khronos](https://docs.vulkan.org/samples/latest/samples/api/timestamp_queries/README.html).
Le budget théorique d’une image à 144 Hz est 6,94 ms ; ces mesures de passe
laissent une marge GPU sur cet échantillon, sans garantir le débit de toute
l’application ni de toutes les scènes.

## Mémoire et distribution

Un masque GPU RGBA8 de 640×360 : 921 600 octets (0,88 Mio).
Les buffers CPU de construction totalisent au plus 3 456 000 octets (3,30 Mio),
libérés après l’upload. Les buffers de staging Vulkan restent à 7 Mio selon
les descripteurs acceptés. Pas de nouvelles planches PNG/JCR artistiques.
Les nouvelles images et la vidéo sont des captures de diagnostic du renderer.

ELF : 276280 octets ; compact UPX : 99068 octets.
Les bibliothèques dynamiques et ressources ne sont pas incluses dans ces tailles.
Les ressources propriétaires originales et le fichier INI sont exclus de l’archive.
Les anciens rapports et captures restent des jalons historiques.

Reproduction manuelle :

```sh
make test
make sanitize
python3 tools/test_material_gpu.py
python3 tools/test_environment_gpu.py
python3 tools/bench_environment.py
```

Le test de capture GPU nécessite Hyprland, grim, Pillow et NumPy. Le benchmark utilise
un exécutable temporaire et l’interface Vulkan interne de la révision Sokol
épinglée ; aucune instrumentation supplémentaire n’est embarquée en production.
Journaux et mesures structurées : `validation/living-*`.
