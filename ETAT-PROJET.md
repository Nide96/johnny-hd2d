# État — Johnny original exclusivement

Le chargement et le pipeline des planches RGBA de Johnny ont été retirés du
renderer. `renderer_create` n’accepte désormais que la palette, le décor et
le palmier. `--johnny-hd` reste accepté pour les anciens lanceurs mais ne charge
plus de personnage généré, même dans `--walk-demo`.

La matière R8 de Johnny réchauffe la peau après fusion linéaire des damiers,
renforce la barbe et conserve des blancs lumineux. Les autres ressources
continuent leur traitement existant. Les sprites composites conservent leurs
éléments et leur palette partagée. Les coordonnées et le canvas VM sont inchangés.

Le rendu vise directement la résolution de sortie. La capture de comparaison
est en 1920×1080 natif ; elle ne constitue pas une mesure FPS. Les tests visuels
ne couvrent qu’un échantillon, les tests de canvas couvrent les 79 scènes.

Lancer `./preview-hybrid.sh --fullscreen` ou `./preview-sleep.sh --fullscreen`.
Validation, journaux et limites : [VALIDATION-UNIFORME.md](VALIDATION-UNIFORME.md).
Les documents HD antérieurs décrivent des jalons archivés et leurs correspondances,
pas les ressources activées par le renderer actuel.

Dernière correction : petits damiers de peau fusionnés en une seule cellule 2×2,
éclats blancs isolés atténués, pas de biseautage sur les transitions de palette
de Johnny, rim-light à 15 %. Les contours conservent leur couverture et la teinte
fusionnée. Comparaison réelle sur STNDLAY3, scène51/jour6/pas709 :
`preview-johnny-sober-detail.png`. Les tests GPU contrôlent un œil, un blanc de
tissu, un éclat isolé et le damier court. Journaux `validation/sober-*`.

Audio raccordé : 23 effets PCM d’origine via Sokol Audio/ALSA, rééchantillonnage
vers la fréquence de sortie, dernier son prioritaire, `--volume 0..100`,
`--mute` et touche M. Aucun périphérique en headless ; erreur audio non fatale.
Le hook dans `vm_core.c` observe chaque pas réussi sans modifier le moteur,
le calendrier, les tirages RNG ni le canvas. [VALIDATION-AUDIO.md](VALIDATION-AUDIO.md).

Diagnostic audio confirmé à l’écoute par l’utilisateur : backend et trois bips
audibles ; pêche audible en reprenant avant le premier bruitage. Le lanceur
`preview-hybrid.sh` utilise désormais `--skip-steps 65` (anciennement 100).
`--audio-test` joue trois bips sans fenêtre, données originales ou VM.

Palmier rétro disponible : `--modern --palm-original` et lanceur
`preview-reading-palm.sh` (gag de lecture/noix de coco, scène 35). Le sprite
MJREAD20 contient sa propre palme ; le palmier entièrement R8 partage sa
géométrie et sa palette, évitant le mélange avec la couronne peinte.
Comparaison historique `preview-palm-original-detail.png`. Depuis le nettoyage
des textures, le mode moderne utilise systématiquement le palmier R8 dès
le lancement et pour toute l’exécution. Voir `VALIDATION-PALMIER-RETRO.md`.

Décor vivant intégré en mode moderne : mer à plusieurs mouvements, écume du
rivage, sable humide, ombre de nuages, vent des palmes protégé par scène,
ciel GPU avec soleil/lune/nuages/étoiles, lueur et braises du vrai feu.
`preview-living.sh` montre un cycle jour/nuit en 120 s ; `preview-stars.sh`
commence de nuit. Horloge visuelle indépendante ; `--freeze-scene` et
`--decor-time` pour les captures. L’identifiant de scène est ajouté aux
métadonnées de présentation pour verrouiller le vent, sans changer la logique
VM. Rapport : `VALIDATION-DECOR-VIVANT.md`.

## Allègement des textures remplacées

Suppression des planches PNG/JCR de Johnny et du palmier peint ; le mode moderne
utilise systématiquement les sprites originaux pour ces éléments. Le décor de
l’île et les données originales restent conservés. Inventaire exact :
`validation/asset-cleanup.json`. Les métadonnées et rapports des planches
retirées sont historiques ; les outils de génération de ces planches ne sont
plus nécessaires au rendu ni à la compilation.
