# Vérification du recalage — 18 octobre 2026

> Historique du jalon de recalage. Les résultats actuels, les tailles de binaire,
> la mémoire de staging et la couverture des actions sont dans `VALIDATION-SCENES.md`.


## Ce qui a été corrigé

La photographie du premier prototype imposait son placement : palmier à gauche,
acteurs retournés et marche décalée. Le rendu utilise maintenant les repères
locaux originaux et une caméra commune, définie dans `layout.h`.

Le tronc original (443,148,24,145) donne (645,189,43,261) à l’écran logique
1280×720. Son pied (455,293) donne (667,450). Le masque vient du vrai sprite
original ; seule l’écorce est moderne. La couronne est échantillonnée séparément,
avec sa jonction alignée sur le même axe. Les commandes d’occultation restent
placées après le personnage quand le moteur demande un passage derrière le tronc.

Les traces de décor sont séparées des commandes du canvas classique : elles ne
modifient ni les pixels de la VM, ni les chemins, ni les horloges, ni le RNG.
La démo ne reçoit plus de translation particulière et aucun miroir de caméra.

## Contrôles exécutés

- Suite C99 stricte et ASan/UBSan : ressources, VM, histoire, INI, captures,
  marche et nouvelle caméra. Aucune erreur, avec détection des fuites désactivée
  comme dans les tests précédents à cause de l’environnement ptrace.
- Reconstruction exacte du canvas C sur les 79 scènes et 30 537 images.
- Marche : 600 images, huit poses, deux sens, trois couches statiques et des
  commandes d’occultation au croisement du tronc ; aucune progression du scénario.
- Caméra : pied et empreinte du tronc, invariance à l’origine variable de l’île,
  sens horizontal conservé.
- Versions normale et compacte : 1 000 pas de scénario, hash inchangé
  `de8bfcfeb48c1c6b`, aucune saturation d’atlas, reconstruction exacte.
- Vraie fenêtre Vulkan sur la 780M : binaires normal et compact en marche HD,
  mode classique et repli nocturne, 240 images présentées par essai, codes de
  sortie 0. Couche de validation Vulkan activée ; aucune erreur. Avertissement
  DPI X11 connu et inchangé.
- Capture de six secondes : 48 PNG à 8 images/s, convertis en GIF/MP4. Ce sont
  des captures de l’application ; cette cadence ne mesure pas les FPS du moteur.
- Zone intérieure fixe du tronc (492,202)..(498,320) dans la capture 950×534 :
  **0 pixel modifié sur 833**, sur les 48 images de la traversée. Le tronc reste
  donc visible lorsque Johnny se trouve derrière ; la vérification visuelle
  confirme le passage.

## Limites

La rive du fond précalculé est recalée mais n’utilise pas encore les masques
exacts des marées. Le masque du tronc et son implantation sont exacts par rapport
au port C99 ; le feuillage est une interprétation artistique. Le test visuel de
l’occultation porte sur la marche. Les interactions ADS/TTM et les accessoires
nécessitent encore une revue visuelle par scène. La comparaison exhaustive des
captures porte sur le canvas C et ses commandes, pas sur une exécution Win16.

La marche v3 affine certains appuis et bras ; elle n’est pas une animation
physiquement contrainte. Les autres actions gardent les sprites d’origine.
Le feu peint dans l’ancienne image a été retiré ; aucun éclairage dynamique
moderne des feux de scénario n’est encore revendiqué.

Binaire ELF : 161 320 octets ; version UPX : 70 240 octets. Images, données et
bibliothèques externes en plus. Le staging Vulkan est porté à 32 Mio pour
l’initialisation des trois textures RGBA ; ce chiffre n’est pas la mémoire totale.
